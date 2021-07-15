// Quantum phase estimation
// Source: ./examples/qpe.cpp
// See also ./examples/circuits/qpe_circuit.cpp for a high-level API example
// (with a 2-qubit target unitary!)

/*
A program to construct the following quantum phase estimator circuit and
execute simulation on the phase of U = diag(1, e^{2*pi*i*theta}).

|0> ----H--------------@----x-------------------R3+--R2+--H----|D---- [q0]
                       |    |                   |    |
|0> ----H---------@----+----+---------R2+--H----+----@---------|D---- [q1]
                  |    |    |         |         |
|0> ----H----@----+----+----x----H----@---------@--------------|D---- [q2]
             |    |    |
|0> ----X----U----U^2--U^4------------------------------------------- [q3]

*/

#include <cmath>
#include <iostream>
#include <numeric>
#include <tuple>
#include <vector>

#include "qpp.h"

int main() {
    using namespace qpp;

    idx nq_c = 3;                            // number of counting qubits
    idx nq_a = 1;                            // number of ancilla qubits
    idx nq = nq_c + nq_a;                    // total number of qubits
    ket psi = mket(std::vector<idx>(nq, 0)); // |0>^\otimes n

    cmat U(2, 2); // initialize a unitary operator
    // we use the T gate as an example; we expect estimated theta = 1/8 (0.125).
    double theta = 0.125; // change if you want, increase n for more precision
    U << 1, 0, 0, std::exp(2 * pi * 1_i * theta);

    ket result = psi;
    std::vector<idx> counting_qubits(nq_c);
    std::iota(counting_qubits.begin(), counting_qubits.end(), 0);
    std::vector<idx> ancilla(nq_a);
    std::iota(ancilla.begin(), ancilla.end(), nq_c);

    std::cout << ">> QPE on nq_c = " << nq_c
              << " counting qubits, nq_a = " << nq_a << " ancilla qubits\n";

    std::cout << ">> The sequence of applied gates is:\n";
    for (idx i = 0; i < counting_qubits.size(); ++i) {
        // apply Hadamard on counting qubits
        result = apply(result, gt.H, {i});
        std::cout << "H" << i << " ";
    }
    // prepare |1>, the second eigenvector of U
    result = apply(result, gt.X, ancilla);
    std::cout << "X" << disp(ancilla, ",") << '\n';

    // apply controlled unitary operations
    idx powerU = 1;
    for (idx i = 0; i < nq_c; ++i) {
        std::cout << "CU(" << nq_c - i - 1 << ", " << disp(ancilla, ", ")
                  << ")^" << powerU << '\n';
        result = applyCTRL(result, U, {nq_c - i - 1}, ancilla);
        U = powm(U, 2);
        powerU *= 2;
    }

    // apply inverse quantum Fourier transform to convert state of the counting
    // register
    result = applyTFQ(result, counting_qubits);
    std::cout << "QFT^{-1}" << disp(counting_qubits, ", ", "(", ")") << "\n";

    // measure the counting register and readout probabilities
    auto measured = measure_seq(result, {counting_qubits});
    auto res = std::get<RES>(measured);
    std::cout << ">> Measurement result [q0 q1 ... ]: " << disp(res, " ");
    std::cout << '\n';

    // decimal representation of the measurement result
    idx decimal = multiidx2n(res, std::vector<idx>(counting_qubits.size(), 2));

    // readout phase estimate
    double theta_e = decimal / std::pow(2, nq_c);
    std::cout << ">> Input theta = " << theta << '\n';
    std::cout << ">> Estimated theta = " << theta_e << '\n';
    std::cout << ">> Norm difference: " << std::abs(theta_e - theta) << '\n';
}
