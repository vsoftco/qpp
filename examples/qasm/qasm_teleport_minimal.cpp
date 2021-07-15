// Minimal teleportation OpenQASM example
// Source: ./examples/qasm/qasm_teleport_minimal.cpp
#include <iostream>

#include "qpp.h"

int main() {
    using namespace qpp;

    // create a qpp::QCircuit from a QASM file
    QCircuit qc = qasm::read_from_file(QPP_ROOT_DIR
                                       "/examples/qasm/teleport_minimal.qasm");

    // we could have also used a C++ standard stream from <fstream>, like below
    // std::ifstream ifs{PATH "/examples/qasm/teleport_minimal.qasm"};
    // QCircuit qc = qasm::read(ifs);

    // note that QASM measurements are non-destructive, so the final state after
    // this step when executed on an engine will be a state of 3 qubits; that is
    // why we discard the first two qubits in the next line
    qc.discard({0, 1});

    // display the quantum circuit and its corresponding resources
    // use qc.to_JSON() for JSON output
    std::cout << qc << "\n\n" << qc.get_resources() << "\n\n";

    QEngine q_engine{qc}; // create an engine out of a quantum circuit

    // execute the quantum circuit
    q_engine.execute();

    // display the measurement statistics
    // use q_engine.to_JSON() for JSON output
    std::cout << q_engine << '\n';

    // displays the final output state
    std::cout << ">> Final state:\n";
    std::cout << disp(q_engine.get_psi()) << '\n';
}
