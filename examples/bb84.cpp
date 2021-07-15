// BB84
// Source: ./examples/BB84.cpp
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

#include "qpp.h"

// (basis, state) pair collection type ; 0 -> Z basis, 1 -> X basis
using bases_states_T = std::vector<std::pair<short, short>>;
using key_T = std::vector<short>; // key type (bit string)

// display Alice's and Bob's bases choices and states
void display(const bases_states_T& Alice_bases_states,
             const bases_states_T& Bob_bases_states) {
    using namespace qpp;
    auto n = Alice_bases_states.size();
    std::cout << "Alice's states: ";
    for (idx i = 0; i < n; ++i)
        std::cout << Alice_bases_states[i].second << ' ';
    std::cout << '\n';
    std::cout << "Alice's bases:  ";
    for (idx i = 0; i < n; ++i)
        std::cout << (Alice_bases_states[i].first == 0 ? 'Z' : 'X') << ' ';
    std::cout << '\n';
    std::cout << "Bob's bases:    ";
    for (idx i = 0; i < n; ++i)
        std::cout << (Bob_bases_states[i].first == 0 ? 'Z' : 'X') << ' ';
    std::cout << '\n';
    std::cout << "Bob's states:   ";
    for (idx i = 0; i < n; ++i)
        std::cout << Bob_bases_states[i].second << ' ';
    std::cout << '\n';
}

// key sifting, removes the locations where the bases do not coincide
void key_sift(bases_states_T& Alice_bases_states,
              bases_states_T& Bob_bases_states) {
    using namespace qpp;
    auto n = Alice_bases_states.size();
    bases_states_T result_A, result_B;
    for (idx i = 0; i < n; ++i) {
        if (Alice_bases_states[i].first != Bob_bases_states[i].first)
            continue;
        result_A.emplace_back(Alice_bases_states[i]);
        result_B.emplace_back(Bob_bases_states[i]);
    }
    Alice_bases_states = result_A;
    Bob_bases_states = result_B;
}

// Alice and Bob check a subset (of size k) of their qubits and estimate how
// much Eve eavesdropped; returns the number of positions where they disagree
double check_eavesdrop(bases_states_T& Alice_bases_states,
                       bases_states_T& Bob_bases_states, qpp::idx k) {
    using namespace qpp;
    auto n = Alice_bases_states.size();

    if (k > n) {
        std::cout << ">> Not enough check qubits (k too large), aborting...\n";
        exit(EXIT_FAILURE);
    }

    std::vector<idx> pos(n);
    std::iota(pos.begin(), pos.end(), 0);
    auto& gen =
#ifdef NO_THREAD_LOCAL_
        RandomDevices::get_instance().get_prng();
#else
        RandomDevices::get_thread_local_instance().get_prng();
#endif
    // first k elements label the qubits we want to check
    std::shuffle(pos.begin(), pos.end(), gen);
    // sort (first k of them) for std::binary_search() later
    std::sort(pos.begin(), std::next(pos.begin(), k));

    bases_states_T result_A, result_B;
    idx cnt = 0; // how many bits differ
    for (idx i = 0; i < n; ++i) {
        // is current position part of the ones Alice and Bob need to check?
        if (std::binary_search(pos.begin(), std::next(pos.begin(), k), i)) {
            auto basis_AB = Alice_bases_states[i].first;
            auto state_A = Alice_bases_states[i].second;
            auto state_B = Bob_bases_states[i].second;

            ket psi_A = (state_A == 0) ? 0_ket : 1_ket;
            ket psi_B = (state_B == 0) ? 0_ket : 1_ket;
            cmat U = gt.Z;     // measurement basis
            if (basis_AB != 0) // if X basis
            {
                U = gt.H;
                psi_A = gt.H * psi_A;
                psi_B = gt.H * psi_B;
            }

            auto measure_A = measure(psi_A, U);
            auto m_A = std::get<RES>(measure_A); // Alice's measurement result

            auto measure_B = measure(psi_B, U);
            auto m_B = std::get<RES>(measure_B); // Bob's measurement result

            if (m_A != m_B)
                ++cnt;
        } else {
            result_A.emplace_back(Alice_bases_states[i]);
            result_B.emplace_back(Bob_bases_states[i]);
        }
    }
    Alice_bases_states = result_A;
    Bob_bases_states = result_B;

    return static_cast<double>(cnt) / static_cast<double>(k);
}

// compute the final key; technically, here is where we do error correction and
// privacy amplification; however, here we simply discard the bits that differ
key_T final_key(const key_T& Alice_raw_key, const key_T& Bob_raw_key) {
    using namespace qpp;
    auto n = Alice_raw_key.size();
    key_T result;
    for (idx i = 0; i < n; ++i) {
        if (Alice_raw_key[i] != Bob_raw_key[i])
            continue;
        result.emplace_back(Alice_raw_key[i]);
    }
    return result;
}

// helper, retrieves the key from a collection of (basis, state) pairs
key_T get_key(const bases_states_T& bases_states) {
    using namespace qpp;
    auto n = bases_states.size();
    key_T result(n);
    for (idx i = 0; i < n; ++i)
        result[i] = bases_states[i].second;
    return result;
}

int main() {
    using namespace qpp;

    idx n = 100;    // no. of qubits Alice sends to Bob
    idx k = 20;     // no. of qubits Alice and Bob check for eavesdropping
    double p = 0.5; // probability of Eve intercepting (and altering) the qubits

    // when we should abort due to eavesdropping; lower in reality
    double abort_rate = 0.2;

    std::cout << ">> BB84, sending n = " << n << " qubits from Alice to Bob\n";
    std::cout << ">> With probability p = " << p
              << ", Eve intercepts the qubits and randomly measures them in the"
                 " Z or X basis, then sends them to Bob\n";
    std::cout << ">> Excludes error correction and privacy amplification\n";

    // Alice's basis and state pairs (basis, state); 0 -> Z basis, 1 -> X
    // basis
    bases_states_T Alice_bases_states(n);
    for (auto& elem : Alice_bases_states) {
        // chose a random basis, 0 -> Z basis, 1 -> X basis
        short basis = bernoulli() ? 0 : 1;
        // chose a random state, |0> or |1> in the basis 'basis'
        short state = bernoulli() ? 0 : 1;
        elem = std::make_pair(basis, state);
    }

    // Bob's basis and state pairs (basis, state); 0 -> Z basis, 1 -> X
    // basis
    bases_states_T Bob_bases_states(n);
    for (auto& elem : Bob_bases_states) {
        // chose a random basis, 0 -> Z basis, 1 -> X basis
        short basis = bernoulli() ? 0 : 1;
        elem.first = basis;
    }

    // Alice "prepares" the qubits and "sends" them to Bob one by one
    // Eve is in the middle and intercepts/resends the qubits with
    // probability p Bob measures the received qubits one by one
    for (idx i = 0; i < n; ++i) {
        auto basis_A = Alice_bases_states[i].first;
        auto state_A = Alice_bases_states[i].second;
        ket psi = (state_A == 0) ? 0_ket : 1_ket;
        if (basis_A != 0) // if X basis
            psi = gt.H * psi;

        // Eve intercepts the qubit and randomly measures it in the Z or X
        // basis, then sends it to Bob
        if (bernoulli(p)) {
            // chose a random basis, 0 -> Z basis, 1 -> X basis
            short basis_E = bernoulli();
            cmat U_E = (basis_E == 0) ? gt.Z : gt.H;
            auto measure_E = measure(psi, U_E);
            auto m_E = std::get<RES>(measure_E); // measurement result
            psi = std::get<ST>(measure_E)[m_E];  // update the state accordingly
        }

        // Bob measures the qubit Eve re-sent
        auto basis_B = Bob_bases_states[i].first;
        // Bob's measurement eigenvectors
        cmat U_B = (basis_B == 0) ? gt.Z : gt.H;
        auto measure_B = measure(psi, U_B);
        auto m_B = std::get<RES>(measure_B); // measurement result
        Bob_bases_states[i].second =
            static_cast<decltype(Bob_bases_states[i].second)>(m_B);
    }

    // display the results before bases sifting
    std::cout << ">> Before sifting\n";
    display(Alice_bases_states, Bob_bases_states);

    // filter on same bases
    key_sift(Alice_bases_states, Bob_bases_states);

    // check eavesdropping
    auto eves_rate = check_eavesdrop(Alice_bases_states, Bob_bases_states, k);
    std::cout << ">> Detected eavesdropping rate: " << eves_rate << '\n';
    // if rate is too high we should abort here
    if (eves_rate > abort_rate) {
        std::cout
            << ">> Detected eavesdropping rate is too high, aborting...\n";
        return EXIT_FAILURE;
    }

    // display the results after basis sifting and eavesdropping detection
    std::cout << ">> After sifting and eavesdrop detection (raw key)\n";
    display(Alice_bases_states, Bob_bases_states);

    std::cout << ">> Established keys\n";
    // display the raw key on Alice's side
    auto raw_key_A = get_key(Alice_bases_states);
    std::cout << "Alice's raw key: " << disp(raw_key_A, " ", "", "") << '\n';

    // display the raw key on Bob's side
    auto raw_key_B = get_key(Bob_bases_states);
    std::cout << "Bob's raw key:   " << disp(raw_key_B, " ", "", "") << '\n';

    // display the final key and the corresponding rate
    auto key = final_key(raw_key_A, raw_key_B);
    std::cout << "Final key:       " << disp(key, " ", "", "") << '\n';

    double key_rate = static_cast<double>(key.size()) / static_cast<double>(n);

    std::cout << ">> Bits/keys sizes: " << n << '/' << raw_key_A.size() << '/'
              << key.size() << '\n';
    std::cout << ">> Final key rate: " << key_rate << '\n';
}
