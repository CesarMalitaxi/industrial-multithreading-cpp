// Industrial sorting simulation - Win32 / C++
// Academic project (Universidad Europea de Madrid, 2026)
// Team: Cesar Malitaxi, Ignacio Barreiro, Luka Kurtovic
// Reviewed version based on the team's original final submission.
// Changes: safe timeout handling, explicit buffer occupancy,
//          correct end-of-work monitoring, and clean output files.
// NOTE: this reviewed version must be compiled and tested on Windows.

#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace std;

const int TAM_BUFFER = 8;

struct Pieza {
    int id;
    char tipo;
    int origen;
};

Pieza buffer[TAM_BUFFER];
int indiceEntrada = 0;
int indiceSalida = 0;
int piezasEnBuffer = 0;
int piezasEnProceso = 0;
int cintasFinalizadas = 0;
int totalProducidas = 0;
int totalProcesadas = 0;
int totalA = 0;
int totalB = 0;
int totalC = 0;

HANDLE hMutex = NULL;
HANDLE hSemHuecos = NULL;
HANDLE hSemPiezas = NULL;

DWORD WINAPI HiloCinta(LPVOID param) {
    int numCinta = *static_cast<int*>(param);
    char nombreArchivo[20];
    sprintf_s(nombreArchivo, sizeof(nombreArchivo), "cinta%d.txt", numCinta);

    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir " << nombreArchivo << endl;
        // Even on input failure, notify consumers that this producer finished.
        WaitForSingleObject(hMutex, INFINITE);
        ++cintasFinalizadas;
        ReleaseMutex(hMutex);
        return 1;
    }

    int idLeido;
    char tipoLeido;
    while (archivo >> idLeido >> tipoLeido) {
        if (tipoLeido != 'A' && tipoLeido != 'B' && tipoLeido != 'C') {
            cerr << "Tipo no reconocido para pieza " << idLeido << endl;
            continue;
        }
        Sleep(1000 + rand() % 2001);  // Simulated conveyor: 1-3 s.
        if (WaitForSingleObject(hSemHuecos, INFINITE) != WAIT_OBJECT_0) break;
        WaitForSingleObject(hMutex, INFINITE);
        buffer[indiceEntrada] = {idLeido, tipoLeido, numCinta};
        indiceEntrada = (indiceEntrada + 1) % TAM_BUFFER;
        ++piezasEnBuffer;
        ++totalProducidas;
        ReleaseMutex(hMutex);
        ReleaseSemaphore(hSemPiezas, 1, NULL);
    }

    WaitForSingleObject(hMutex, INFINITE);
    ++cintasFinalizadas;
    ReleaseMutex(hMutex);
    return 0;
}

DWORD WINAPI HiloRobot(LPVOID) {
    while (true) {
        // Only remove a piece when the semaphore confirms availability.
        DWORD res = WaitForSingleObject(hSemPiezas, 300);
        if (res == WAIT_TIMEOUT) {
            WaitForSingleObject(hMutex, INFINITE);
            bool fin = cintasFinalizadas == 2 && piezasEnBuffer == 0;
            ReleaseMutex(hMutex);
            if (fin) break;
            continue;  // Do not consume an uninitialized buffer entry.
        }
        if (res != WAIT_OBJECT_0) return 1;

        WaitForSingleObject(hMutex, INFINITE);
        Pieza p = buffer[indiceSalida];
        indiceSalida = (indiceSalida + 1) % TAM_BUFFER;
        --piezasEnBuffer;
        ++piezasEnProceso;
        ReleaseMutex(hMutex);
        ReleaseSemaphore(hSemHuecos, 1, NULL);

        Sleep(1000 + rand() % 1001);  // Simulated robot: 1-2 s.

        char nombreSalida[20];
        sprintf_s(nombreSalida, sizeof(nombreSalida), "Cont%c.txt", p.tipo);
        WaitForSingleObject(hMutex, INFINITE);
        ofstream salida(nombreSalida, ios::app);
        if (salida.is_open()) {
            salida << p.id << '\t' << p.origen << '\n';
            ++totalProcesadas;
            if (p.tipo == 'A') ++totalA;
            else if (p.tipo == 'B') ++totalB;
            else if (p.tipo == 'C') ++totalC;
        } else {
            cerr << "No se pudo escribir " << nombreSalida << endl;
        }
        --piezasEnProceso;
        ReleaseMutex(hMutex);
    }
    return 0;
}

DWORD WINAPI HiloMonitor(LPVOID) {
    while (true) {
        Sleep(5000);
        WaitForSingleObject(hMutex, INFINITE);
        cout << "\n--- ESTADO DEL SISTEMA ---\n";
        cout << "Piezas clasificadas: " << totalProcesadas << endl;
        if (totalProcesadas > 0) {
            cout << "Tipo A: " << 100.0 * totalA / totalProcesadas << "%\n";
            cout << "Tipo B: " << 100.0 * totalB / totalProcesadas << "%\n";
            cout << "Tipo C: " << 100.0 * totalC / totalProcesadas << "%\n";
        }
        bool fin = (cintasFinalizadas == 2 && piezasEnBuffer == 0 && piezasEnProceso == 0);
        ReleaseMutex(hMutex);
        if (fin) break;
    }
    return 0;
}

int main() {
    srand(static_cast<unsigned>(time(NULL)));

    // The input files must be in the working directory.
    ifstream prueba1("cinta1.txt"), prueba2("cinta2.txt");
    if (!prueba1.is_open() || !prueba2.is_open()) {
        cerr << "Coloque cinta1.txt y cinta2.txt junto al programa.\n";
        return 1;
    }
    prueba1.close();
    prueba2.close();

    // Start each run with clean output files (no duplicate records from older runs).
    ofstream("ContA.txt", ios::trunc).close();
    ofstream("ContB.txt", ios::trunc).close();
    ofstream("ContC.txt", ios::trunc).close();

    hMutex = CreateMutex(NULL, FALSE, NULL);
    hSemHuecos = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, NULL);
    hSemPiezas = CreateSemaphore(NULL, 0, TAM_BUFFER, NULL);
    if (!hMutex || !hSemHuecos || !hSemPiezas) {
        cerr << "Error creando objetos de sincronizacion Win32.\n";
        return 1;
    }

    int idsCintas[2] = {1, 2};
    HANDLE hilos[5] = {
        CreateThread(NULL, 0, HiloCinta, &idsCintas[0], 0, NULL),
        CreateThread(NULL, 0, HiloCinta, &idsCintas[1], 0, NULL),
        CreateThread(NULL, 0, HiloRobot, NULL, 0, NULL),
        CreateThread(NULL, 0, HiloRobot, NULL, 0, NULL),
        CreateThread(NULL, 0, HiloMonitor, NULL, 0, NULL)
    };
    for (int i = 0; i < 5; ++i) {
        if (!hilos[i]) {
            cerr << "No se pudo crear uno de los hilos.\n";
            return 1;
        }
    }

    WaitForMultipleObjects(5, hilos, TRUE, INFINITE);
    for (int i = 0; i < 5; ++i) CloseHandle(hilos[i]);
    CloseHandle(hMutex);
    CloseHandle(hSemHuecos);
    CloseHandle(hSemPiezas);

    cout << "\nSimulacion finalizada. " << totalProcesadas << " de "
         << totalProducidas << " piezas clasificadas.\n";
    return totalProcesadas == totalProducidas ? 0 : 1;
}
