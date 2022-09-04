#ifndef LIBHOST_RUNTIME_H
#define LIBHOST_RUNTIME_H

#include "xcl2/xcl2.hpp"

#include <unordered_map>

// 4k aligned vector
template<typename T>
using aligned_vector = std::vector<T, aligned_allocator<T> >;

// config words
enum MigrateDirection {ToDevice, ToHost};
enum BufferType {ReadOnly, WriteOnly, ReadWrite};
enum ExecTarget {SW_EMU, HW_EMU, HW};

ExecTarget check_target() {
    if(xcl::is_emulation()) {
        if(xcl::is_hw_emulation())
            return ExecTarget::HW_EMU;
        return ExecTarget::SW_EMU;
    }
    return ExecTarget::HW;
}

// RunTime class: helps to manage kernels and buffers
class RunTime {
private:
    cl::Device _device;
    cl::Program _program;
    cl_int _errflag;
    std::unordered_map<std::string, cl_mem_ext_ptr_t> _ext_ptrs;

public:
    cl::Context context;
    cl::CommandQueue command_q;
    std::unordered_map<std::string, cl::Kernel> kernels;
    std::unordered_map<std::string, cl::Buffer> buffers;

    // constructor
    RunTime (
        std::string device_name,
        std::string xclbin_path,
        std::vector<std::string> kernel_list
    ) {
        // find device
        bool found_device = false;
        auto devices = xcl::get_xil_devices();
        for (size_t i = 0; i < devices.size(); i++) {
            if (devices[i].getInfo<CL_DEVICE_NAME>() == device_name) {
                this->_device = devices[i];
                found_device = true;
                break;
            }
        }
        if (!found_device) {
            std::cout << "[ERROR]: Failed to find " << device_name << ", exit!" << std::endl;
            exit(EXIT_FAILURE);
        }

        // setup run time context
        this->context = cl::Context(this->_device, NULL, NULL, NULL);

        // read FPGA binary
        auto file_buf = xcl::read_binary_file(xclbin_path);
        cl::Program::Binaries binaries{{file_buf.data(), file_buf.size()}};
        OCL_CHECK(this->_errflag, this->_program = cl::Program(context, {this->_device}, binaries, NULL, &(this->_errflag)));

        // create command queue
        OCL_CHECK(
            this->_errflag,
            this->command_q = cl::CommandQueue(
                    this->context,
                    this->_device,
                    CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE,
                    &(this->_errflag)
            )
        );

        // create kernels
        for (auto kn : kernel_list) {
            OCL_CHECK(
                this->_errflag,
                this->kernels[kn] = cl::Kernel(this->_program, kn.c_str(), &(this->_errflag))
            );
        }
    }

    // data transfer functions
    void nb_migrate_data(std::vector<std::string> buffer_list, MigrateDirection direction) {
        switch (direction) {
            case ToDevice:
                for (auto bn : buffer_list) {
                    OCL_CHECK(
                        this->_errflag,
                        this->_errflag = this->command_q.enqueueMigrateMemObjects(
                            {this->buffers[bn]},
                            0
                        )
                    );
                }
            break;
            case ToHost:
                for (auto bn : buffer_list) {
                    OCL_CHECK(
                        this->_errflag,
                        this->_errflag = this->command_q.enqueueMigrateMemObjects(
                            {this->buffers[bn]},
                            CL_MIGRATE_MEM_OBJECT_HOST
                        )
                    );
                }
            break;
            default:
                std::cout << "[ERROR]: Unsupported data migration direction!"<< std::endl;
            break;
        }
    }

    void migrate_data(std::vector<std::string> buffer_list, MigrateDirection direction) {
        this->nb_migrate_data(buffer_list, direction);
        this->command_q.finish();
    }

    // memory management functions
    void create_buffer(
        std::string name, void *data_ptr, size_t size_in_bytes, BufferType type,
        const int memory_channel_id
    ) {
        cl_mem_ext_ptr_t eptr;
        eptr.flags = memory_channel_id;
        eptr.obj = data_ptr;
        eptr.param = 0;
        this->_ext_ptrs[name] = eptr;
        switch (type) {
            case ReadOnly:
                OCL_CHECK(
                    this->_errflag, this->buffers[name] = cl::Buffer(
                        this->context,
                        CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                        size_in_bytes,
                        &(this->_ext_ptrs[name]),
                        &(this->_errflag)
                    );
                )
            break;
            case WriteOnly:
                OCL_CHECK(
                    this->_errflag, this->buffers[name] = cl::Buffer(
                        this->context,
                        CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                        size_in_bytes,
                        &(this->_ext_ptrs[name]),
                        &(this->_errflag)
                    );
                )
            break;
            case ReadWrite:
                OCL_CHECK(
                    this->_errflag, this->buffers[name] = cl::Buffer(
                        this->context,
                        CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                        size_in_bytes,
                        &(this->_ext_ptrs[name]),
                        &(this->_errflag)
                    );
                )
            default:
                std::cout << "[ERROR]: Unsupported buffer type!"<< std::endl;
            break;
        }
    }


};

#endif //LIBHOST_RUNTIME_H
