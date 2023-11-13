#include "devices.h"
#include "runtime.h"

#include "vvadd.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <algorithm>

#include "xcl2.hpp"

//--------------------------------------------------------------------------------------------------
// Utilities
//--------------------------------------------------------------------------------------------------
bool check_aligned_vector(
    aligned_vector<int> v,
    aligned_vector<int> ref,
    bool stop_on_mismatch = true
) {
    // check length
    if (v.size() != ref.size()) {
        std::cout << "[ERROR]: Size mismatch!" << std::endl;
        std::cout << "         Expected: " << ref.size() << std::endl;
        std::cout << "         Actual  : " << v.size() << std::endl;
        return false;
    }

    // check items
    bool match = true;
    for (size_t i = 0; i < v.size(); i++) {
        if (v[i] != ref[i]) {
            std::cout << "[ERROR]: Value mismatch!" << std::endl;
            std::cout << "         at i = " << i << std::endl;
            std::cout << "         Expected: " << ref[i] << std::endl;
            std::cout << "         Actual  : " << v[i] << std::endl;
            if (stop_on_mismatch) return false;
            match = false;
        }
    }
    return match;
}

//--------------------------------------------------------------------------------------------------
// Testbench
//--------------------------------------------------------------------------------------------------
int main(int argc, char** argv) {
    // parse arguments
    if(argc != 2) {
        std::cout << "Usage : " << argv[0] << "<xclbin path>" << std::endl;
        std::cout << "Aborting..." << std::endl;
        return 0;
    }
    std::string xclbin_path = argv[1];

    // set the environment variable
    ExecTarget target = check_target();
    if (target == SW_EMU) {
        setenv("XCL_EMULATION_MODE", "sw_emu", true);
    } else if (target == HW_EMU) {
        setenv("XCL_EMULATION_MODE", "hw_emu", true);
    }

    // setup runtime
    //RunTime rt(
    //     u280::name,
    //     xclbin_path,
    //     {"vvadd"}
    // );
    RunTime rt;
    rt.program_device(
        alveo::u280::name,
        alveo::u280::board_xsa,
        xclbin_path
    );


    std::unordered_map<std::string, int> map{{"in1", 0}, {"in2", 1}, {"out", 2}, {"DATA_SIZE", 3}};
    std::vector<std::unordered_map<std::string, int>> kernel_arg_map(1, map);
    rt.create_kernels({"vvadd"}, kernel_arg_map);

    // generate inputs/outputs
    aligned_vector<int> in1(DATA_SIZE);
    aligned_vector<int> in2(DATA_SIZE);
    aligned_vector<int> out_ref(DATA_SIZE);
    aligned_vector<int> out(DATA_SIZE);

    std::generate(
        in1.begin(), in1.end(),
        [](){return rand() % 10;}
    );
    std::generate(
        in2.begin(), in2.end(),
        [](){return rand() % 10;}
    );
    for (size_t i = 0; i < out_ref.size(); i++) {
        out_ref[i] = in1[i] + in2[i];
    }

    // create buffers
    rt.create_buffer(
        "in1", in1.data(), sizeof(int) * DATA_SIZE, BufferType::ReadOnly, alveo::u280::HBM[0]
    );
    rt.create_buffer(
        "in2", in2.data(), sizeof(int) * DATA_SIZE, BufferType::ReadOnly, alveo::u280::HBM[1]
    );
    rt.create_buffer(
        "out", out.data(), sizeof(int) * DATA_SIZE, BufferType::WriteOnly, alveo::u280::HBM[2]
    );

    // migrate data
    rt.migrate_data({"in1", "in2"}, ToDevice);

    // set kernel arguments
    cl_int err;
    OCL_CHECK(err, err = rt.kernels["vvadd"].setArg(0, rt.buffers["in1"]));
    OCL_CHECK(err, err = rt.kernels["vvadd"].setArg(1, rt.buffers["in2"]));
    OCL_CHECK(err, err = rt.kernels["vvadd"].setArg(2, rt.buffers["out"]));
    OCL_CHECK(err, err = rt.kernels["vvadd"].setArg(3, DATA_SIZE));

    // run the kernel!
    rt.command_q.enqueueTask(rt.kernels["vvadd"]);
    rt.command_q.finish();

    // collect results
    rt.migrate_data({"out"}, ToHost);

    // verify
    bool pass = check_aligned_vector(out, out_ref);
    std::cout << (pass ? "[INFO]: Test Passed !" : "[ERROR]: Test Failed!") << std::endl;

    return pass ? 0 : 1;
}
