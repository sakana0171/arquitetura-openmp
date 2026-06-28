#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include <omp.h>
#include <string>
#include <bits/stdc++.h>

using namespace std;

// Converte coordenadas 2D para índice 1D
int indice(int linha, int coluna, int N)
{
    return linha * N + coluna;
}


double programa(int N, int passos, int thread, bool otimizado, string escalonamento, int colapso, bool usaFor, bool is_sequencial, double &checksum_out);

int main(int argc, char *argv[])
{
    vector<int> tamanhos = {512, 1024, 2048, 4096};
    vector<int> passos = {100, 500, 1000};
    vector<string> escalonamento = {"static", "dynamic", "guided"};
    vector<int> treads = {1, 2, 4, 8};
    vector<int> colapso = {1, 2};
    vector<bool> otimizado = {false, true}; 
    vector<bool> usaFor = {false, true};    

    // Abre o arquivo com extensão .csv
    ofstream file("resultados_simulacao.csv");
    
    // Escreve o cabeçalho do CSV
    file << "Tamanho,Passos,Otimizado,Threads,Escalonamento,Collapse,OmpFor,Tempo_s,Speedup,Eficiencia,Checksum\n";
    
    // Configura precisão das casas decimais no arquivo
    file << fixed << setprecision(6);
    cout << fixed << setprecision(6);

    cout << "Iniciando bateria de testes...\n\n";

    
    for (int passo : passos)
    {
        for (int tamanho : tamanhos)
        {
            for (bool otim : otimizado)
            {
                
                cout << "Rodando Baseline Sequencial: N=" << tamanho << ", Passos=" << passo << ", Otimizado=" << otim << "...\n";
                
                double checksum_seq;
                double T1 = programa(tamanho, passo, 1, otim, "none", 1, false, true, checksum_seq);

                // Grava os dados do sequencial no CSV
                // Speedup = 1.0 e Eficiência = 1.0 por definição
                file << tamanho << "," << passo << "," << otim << ",1,Sequencial,1,0," 
                     << T1 << ",1.0,1.0," << checksum_seq << "\n";

                // TESTES PARALELOS
                for (int t : treads)
                {
                    for (string esc : escalonamento)
                    {
                        for (int col : colapso)
                        {
                            for (bool uFor : usaFor)
                            {
                                double checksum_par;
                                double Tp = programa(tamanho, passo, t, otim, esc, col, uFor, false, checksum_par);

                                
                                double speedup = T1 / Tp;
                                double eficiencia = speedup / t;

                                
                                file << tamanho << "," << passo << "," << otim << "," << t << "," 
                                     << esc << "," << col << "," << uFor << "," 
                                     << Tp << "," << speedup << "," << eficiencia << "," << checksum_par << "\n";
                                
                                cout << "Paralelo executado: Threads=" << t << " | Tempo=" << Tp << "s | Speedup=" << speedup << "\n";
                            }
                        }
                    }
                }
                cout << "--------------------------------------------------\n";
            }
        }
    }

    file.close();
    cout << "\nTestes finalizados com sucesso! Arquivo 'resultados_simulacao.csv' gerado.\n";

    return 0;
}

double programa(int N, int passos, int thread, bool otimizado, string escalonamento, int colapso, bool usaFor, bool is_sequencial, double &checksum_out)
{
    vector<double> atual(N * N, 0.0);
    vector<double> proxima(N * N, 0.0);

    // Configurando o Escalonamento em Tempo de Execução (se não for sequencial)
    if (!is_sequencial)
    {
        if (escalonamento == "static")
            omp_set_schedule(omp_sched_static, 0);
        else if (escalonamento == "dynamic")
            omp_set_schedule(omp_sched_dynamic, 0);
        else if (escalonamento == "guided")
            omp_set_schedule(omp_sched_guided, 0);
    }

    // Define uma fonte de calor no centro
    int centro = N / 2;
    atual[indice(centro, centro, N)] = 1000.0;

    // Define bordas fixas com temperatura 100
    for (int i = 0; i < N; i++)
    {
        atual[indice(0, i, N)] = 100.0;
        atual[indice(N - 1, i, N)] = 100.0;
        atual[indice(i, 0, N)] = 100.0;
        atual[indice(i, N - 1, N)] = 100.0;
    }

    auto inicio = chrono::high_resolution_clock::now();

    
    for (int passo = 0; passo < passos; passo++)
    {
        int tamanho_util = N - 2; 

        if (is_sequencial)
        {
            // Versão Sequencial Pura (Baseline para T1)
            for (int linha = 1; linha < N - 1; linha++)
            {
                for (int coluna = 1; coluna < N - 1; coluna++)
                {
                    proxima[indice(linha, coluna, N)] = 0.25 * (atual[indice(linha - 1, coluna, N)] + atual[indice(linha + 1, coluna, N)] + atual[indice(linha, coluna - 1, N)] + atual[indice(linha, coluna + 1, N)]);
                }
            }
        }
        else
        {
            
            if (colapso == 2)
            {
                if (usaFor)
                {
                    #pragma omp parallel for schedule(runtime) num_threads(thread) collapse(2)
                    for (int linha = 1; linha < N - 1; linha++)
                    {
                        for (int coluna = 1; coluna < N - 1; coluna++)
                        {
                            proxima[indice(linha, coluna, N)] = 0.25 * (atual[indice(linha - 1, coluna, N)] + atual[indice(linha + 1, coluna, N)] + atual[indice(linha, coluna - 1, N)] + atual[indice(linha, coluna + 1, N)]);
                        }
                    }
                }
                else
                {
                    #pragma omp parallel num_threads(thread) // como o schedule e o collapse é restrito ao for, não pode ser usado sem ele, assim seria equivalente a fazer um for num treadas com schedule static e sem collapse
                    {
                        int id = omp_get_thread_num();
                        int nt = omp_get_num_threads();

                        long long total_iteracoes = (long long)tamanho_util * tamanho_util;
                        long long iter_por_thread = total_iteracoes / nt;
                        long long resto = total_iteracoes % nt;

                        long long start_iter = id * iter_por_thread + (id < resto ? id : resto);
                        long long end_iter = start_iter + iter_por_thread + (id < resto ? 1 : 0);

                        for (long long iter = start_iter; iter < end_iter; iter++)
                        {
                            int linha = 1 + (iter / tamanho_util);
                            int coluna = 1 + (iter % tamanho_util);
                            proxima[indice(linha, coluna, N)] = 0.25 * (atual[indice(linha - 1, coluna, N)] + atual[indice(linha + 1, coluna, N)] + atual[indice(linha, coluna - 1, N)] + atual[indice(linha, coluna + 1, N)]);
                        }
                    }
                }
            }
            else // colapso == 1
            { 
                if (usaFor)
                {
                    #pragma omp parallel for schedule(runtime) num_threads(thread)
                    for (int linha = 1; linha < N - 1; linha++)
                    {
                        for (int coluna = 1; coluna < N - 1; coluna++)
                        {
                            proxima[indice(linha, coluna, N)] = 0.25 * (atual[indice(linha - 1, coluna, N)] + atual[indice(linha + 1, coluna, N)] + atual[indice(linha, coluna - 1, N)] + atual[indice(linha, coluna + 1, N)]);
                        }
                    }
                }
                else
                {
                    #pragma omp parallel num_threads(thread)
                    {
                        int id = omp_get_thread_num();
                        int nt = omp_get_num_threads();

                        int linhas_por_thread = tamanho_util / nt;
                        int resto = tamanho_util % nt;

                        int start_linha = 1 + id * linhas_por_thread + (id < resto ? id : resto);
                        int end_linha = start_linha + linhas_por_thread + (id < resto ? 1 : 0);

                        for (int linha = start_linha; linha < end_linha; linha++)
                        {
                            for (int coluna = 1; coluna < N - 1; coluna++)
                            {
                                proxima[indice(linha, coluna, N)] = 0.25 * (atual[indice(linha - 1, coluna, N)] + atual[indice(linha + 1, coluna, N)] + atual[indice(linha, coluna - 1, N)] + atual[indice(linha, coluna + 1, N)]);
                            }
                        }
                    }
                }
            }
        }

        // Reaplica as bordas fixas
        for (int i = 0; i < N; i++)
        {
            proxima[indice(0, i, N)] = 100.0;
            proxima[indice(N - 1, i, N)] = 100.0;
            proxima[indice(i, 0, N)] = 100.0;
            proxima[indice(i, N - 1, N)] = 100.0;
        }

        if (otimizado)
        {
            atual.swap(proxima);
        }
        else
        {
            atual = proxima;
        }
    }

    auto fim = chrono::high_resolution_clock::now();
    chrono::duration<double> intervalo = fim - inicio;

    double checksoma = 0.0;
    for (double valor : atual)
    {
        checksoma += valor;
    }

    checksum_out = checksoma;
    return intervalo.count();
}