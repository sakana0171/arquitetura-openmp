#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdlib>

// Converte coordenadas 2D para índice 1D
int indice(int linha, int coluna, int N) {
    return linha * N + coluna;
}

int main(int argc, char* argv[]) {
    int N = 2000;          // tamanho da matriz N x N
    int passos = 1000;     // número de iterações temporais

    // Permite alterar N e passos pela linha de comando
    // Exemplo: ./calor_seq 4096 500
    if (argc >= 2) N = std::atoi(argv[1]);
    if (argc >= 3) passos = std::atoi(argv[2]);

    std::vector<double> atual(N * N, 0.0);
    std::vector<double> proxima(N * N, 0.0);

    // Define uma fonte de calor no centro
    int centro = N / 2;
    atual[indice(centro, centro, N)] = 1000.0;

    // Define bordas fixas com temperatura 100
    for (int i = 0; i < N; i++) {
        atual[indice(0, i, N)] = 100.0;
        atual[indice(N - 1, i, N)] = 100.0;
        atual[indice(i, 0, N)] = 100.0;
        atual[indice(i, N - 1, N)] = 100.0;
    }

    auto inicio = std::chrono::high_resolution_clock::now();

    // Loop temporal: cada passo depende do estado produzido no passo anterior
    for (int passo = 0; passo < passos; passo++) {

        // Atualiza apenas as células internas
        for (int linha = 1; linha < N - 1; linha++) {
            for (int coluna = 1; coluna < N - 1; coluna++) {

                proxima[indice(linha, coluna, N)] = 0.25 * (
                    atual[indice(linha - 1, coluna, N)] +
                    atual[indice(linha + 1, coluna, N)] +
                    atual[indice(linha, coluna - 1, N)] +
                    atual[indice(linha, coluna + 1, N)]
                );
            }
        }

        // Reaplica as bordas fixas
        for (int i = 0; i < N; i++) {
            proxima[indice(0, i, N)] = 100.0;
            proxima[indice(N - 1, i, N)] = 100.0;
            proxima[indice(i, 0, N)] = 100.0;
            proxima[indice(i, N - 1, N)] = 100.0;
        }

        // Atualiza a matriz atual copiando a próxima
        // Esta linha é propositalmente simples, mas pode ser otimizada.
        atual = proxima;
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tempo = fim - inicio;

    // Checksum para comparar versões diferentes
    double checksum = 0.0;
    for (double valor : atual) {
        checksum += valor;
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "N = " << N << "\n";
    std::cout << "Passos = " << passos << "\n";
    std::cout << "Checksum = " << checksum << "\n";
    std::cout << "Tempo = " << tempo.count() << " s\n";

    return 0;
}
