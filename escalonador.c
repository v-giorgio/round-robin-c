#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int bool;
#define true 1
#define false 0

#define HIGH_CONSTANT 100
#define QUANTUM 5
#define DISCO 1
#define FITA 2
#define IMPRESSORA 3
#define DISCO_IO_DURATION 4
#define FITA_IO_DURATION 6
#define IMPRESSORA_IO_DURATION 10
#define MAX_EXEC_TIME 50
#define MAX_QUEUE_SIZE 100

typedef struct {
    char pid[3];
    int priority; // 0 (baixa) e 1 (alta); começa como 1 pois processos novos vão para alta prioridade
    int status; // 0: Pronto, 1: Bloqueado, 2: Finalizado; inicia como pronto = 0
    int duration;
    int total_exec; // tempo total de execucao
    int activation_time; // instante de ativacao
    int end_time;
    int num_ios;    // num total de ios
    int io_type[10]; // array com os TIPOS de io
    int io_time[10]; // array com os INSTANTES de ativacao dos ios
    int io_return; // instante que retorna do io atual
    bool io_concluded[10]; // array com os STATUS de cada io (concluido ou nao)
    int index_io_exec; // indice do io que esta sendo executado
} Process;

// retorna o tempo para cada io type:
int getIOTime(int type) {
    if (type == DISCO) return DISCO_IO_DURATION;
    if (type == FITA) return FITA_IO_DURATION;
    if (type == IMPRESSORA) return IMPRESSORA_IO_DURATION;
}

// Função de comparação para qsort
int compareByReturnTime(const void *a, const void *b) {
    const Process *structA = (const Process *)a;
    const Process *structB = (const Process *)b;

    return structA->io_return - structB->io_return;
}

Process initializeProcess(char pid[], int activation_time, int execution_time,
                           int num_ios, int io_types[], int io_times[]) {
    Process process;
    strcpy(process.pid, pid);
    process.priority = 1;
    process.status = 0;
    process.duration = execution_time;
    process.activation_time = activation_time;
    process.total_exec = 0;
    process.num_ios = num_ios;
    process.end_time = 0;
    process.index_io_exec = 0;
    process.io_return = HIGH_CONSTANT;

    // copia os tipos e instantes de I/O
    for (int i = 0; i < num_ios; i++) {
        process.io_type[i] = io_types[i];
        process.io_time[i] = io_times[i];
        process.io_concluded[i] = false;
    }

    return process;
}

Process* roundRobinScheduler(Process processes[], int num_processes) {
    int time = 0;
    int start_time = 0;
    // inicializar 3 filas
    Process high_priority_queue[MAX_QUEUE_SIZE];
    Process low_priority_queue[MAX_QUEUE_SIZE];
    Process io_queue[MAX_QUEUE_SIZE];

    // inicializar strings de IO para output
    char disco[] = "Disco";
    char fita[] = "Fita";
    char impressora[] = "Impressora";

    int high_priority_queue_size = 0;
    int low_priority_queue_size = 0;

    int lower_exec_time = MAX_EXEC_TIME;
    int first_process_index = -1;

    // adiciona o processo com instante de ativacao mais curto
    for (int i = 0; i < num_processes; i++){
        if (processes[i].activation_time <= lower_exec_time) {
            lower_exec_time = processes[i].activation_time;
            first_process_index = i;
        }
    }
    high_priority_queue[high_priority_queue_size++] = processes[first_process_index];
    time += lower_exec_time;

    printf("\n>>>>>>> Inicio de novo ciclo de execucao no tempo: %d u.t.\n\n", time);

    // marcadores para posicoes das filas (para manter registro da fila no final)
    int item_high_queue = 0;
    int item_lower_queue = 0;
    int num_items_io_queue = 0;
    int temp_diff = 0;

    int index = 0;

    while(true) {
        // CICLO DE EXECUCAO:

        start_time = time;
        if (item_high_queue < high_priority_queue_size) {
            // verifica fila de alta prioridade primeiro:
            index = item_high_queue;
            if (high_priority_queue[index].num_ios > 0 && 
            high_priority_queue[index].io_time[high_priority_queue[index].index_io_exec] <= (high_priority_queue[index].total_exec + QUANTUM)) {
                // chama io antes do quantum acabar:
                temp_diff = high_priority_queue[index].io_time[high_priority_queue[index].index_io_exec] - high_priority_queue[index].total_exec;
                high_priority_queue[index].io_return = getIOTime(high_priority_queue[index].io_type[high_priority_queue[index].index_io_exec]) + time + temp_diff; // armazena o instante que retornara de io
                high_priority_queue[index].total_exec += temp_diff;
                high_priority_queue[index].num_ios--;
                high_priority_queue[index].status = 1;
                io_queue[num_items_io_queue++] = high_priority_queue[index]; // adiciona processo na fila de ios
                time += temp_diff;
            } else if (
            (high_priority_queue[index].num_ios > 0 && high_priority_queue[index].io_time[high_priority_queue[index].index_io_exec] > (high_priority_queue[index].total_exec + QUANTUM)) || 
            (high_priority_queue[index].num_ios == 0 && high_priority_queue[index].total_exec + QUANTUM < high_priority_queue[index].duration)) {
                // sofre preempcao:
                high_priority_queue[index].total_exec += QUANTUM;
                low_priority_queue[low_priority_queue_size++] = high_priority_queue[index]; // adiciona na fila de baixa prioridade
                low_priority_queue[low_priority_queue_size].priority = 0;
                time += QUANTUM;
            } else if (high_priority_queue[index].num_ios == 0 && high_priority_queue[index].total_exec + QUANTUM >= high_priority_queue[index].duration) {
                // processo finalizou:
                time += (high_priority_queue[index].duration - high_priority_queue[index].total_exec);
                high_priority_queue[index].total_exec = high_priority_queue[index].duration;
                high_priority_queue[index].status = 2;
                high_priority_queue[index].end_time = time;
            }
            item_high_queue++; // passar para o prox processo da fila de alta prioridade
        } else if (item_lower_queue < low_priority_queue_size) {
            // verifica fila de baixa prioridade somente se a fila de alta prioridade estiver vazia:
            index = item_lower_queue;
            if (low_priority_queue[index].num_ios > 0 && 
            low_priority_queue[index].io_time[low_priority_queue[index].index_io_exec] <= (low_priority_queue[index].total_exec + QUANTUM)) {
                // chama io antes do quantum acabar:
                temp_diff = low_priority_queue[index].io_time[low_priority_queue[index].index_io_exec] - low_priority_queue[index].total_exec;
                low_priority_queue[index].io_return = getIOTime(low_priority_queue[index].io_type[low_priority_queue[index].index_io_exec]) + time + temp_diff; // armazena o tempo que retornara de io
                low_priority_queue[index].total_exec += temp_diff;
                low_priority_queue[index].num_ios--;
                low_priority_queue[index].status = 1;
                io_queue[num_items_io_queue++] = low_priority_queue[index]; // adiciona processo na fila de ios
                time += temp_diff;
            } else if (
            (low_priority_queue[index].num_ios > 0 && low_priority_queue[index].io_time[low_priority_queue[index].index_io_exec] > (low_priority_queue[index].total_exec + QUANTUM)) || 
            (low_priority_queue[index].num_ios == 0 && low_priority_queue[index].total_exec + QUANTUM < low_priority_queue[index].duration)) {
                // sofre preempcao:
                low_priority_queue[index].total_exec += QUANTUM;
                low_priority_queue[low_priority_queue_size++] = low_priority_queue[index]; // adiciona na fila de baixa prioridade
                time += QUANTUM;
            } else if (low_priority_queue[index].num_ios == 0 && low_priority_queue[index].total_exec + QUANTUM >= low_priority_queue[index].duration) {
                // processo finalizou:
                time += (low_priority_queue[index].duration - low_priority_queue[index].total_exec);
                low_priority_queue[index].total_exec = low_priority_queue[index].duration;
                low_priority_queue[index].status = 2;
                low_priority_queue[index].end_time = time;
            }
            item_lower_queue++; // passar para o prox processo da fila de baixa prioridade
        } else if (num_items_io_queue > 0) {
            // verifica se ha processos em io somente se filas de alta e baixa estiverem vazias (cpu ociosa):
            // iterar pela fila de ios para achar o tempo de retorno de io dos processos e ver qual o mais proximo para somar o 'time' ate ele
            int selected_io = HIGH_CONSTANT;
            for (int i = 0; i < num_items_io_queue; i++) {
                if (io_queue[i].io_concluded[io_queue[i].index_io_exec] == false) {
                    if (io_queue[i].io_return < selected_io) {
                        selected_io = i;
                    }
                }
            }

            if (selected_io < HIGH_CONSTANT) {
                time = io_queue[selected_io].io_return; // atualiza o tempo para quando a cpu voltara a ser utilizada
                printf("> > > [!] Nao houve execucao (CPU ociosa) no periodo: %d u.t. -> %d u.t.\n\n", start_time, time);
            } else {
                // nao havendo processos em mais nenhuma fila, finaliza escalonamento
                printf("\nTodos os processos foram concluidos.\n");
                Process *processes_after_exec = (Process*)malloc(num_processes * sizeof(Process));
                int index_proc = 0;
                // iterar pelas filas para recuperar os processos finalizados com todas as infos
                for (int i = 0; i <= item_high_queue; i++) {
                    if (high_priority_queue[i].status == 2) {
                        processes_after_exec[index_proc++] = high_priority_queue[i];
                    }
                }

                for (int i = 0; i <= item_lower_queue; i++) {
                    if (low_priority_queue[i].status == 2) {
                        processes_after_exec[index_proc++] = low_priority_queue[i];
                    }
                }

                return processes_after_exec;
            }
        }

        // REORGANIZAR FILAS:

        // verifica novos prontos
        bool valid;
        for (int i = 0; i < num_processes; i++) {
            valid = true;
            
            // verifica se já não está nas filas:
            for (int j = 0; j < high_priority_queue_size; j++) {
                if (strcmp(high_priority_queue[j].pid, processes[i].pid) == 0) {
                    valid = false;
                    break;
                }
            }
            for (int j = 0; j < low_priority_queue_size; j++) {
                if (strcmp(low_priority_queue[j].pid, processes[i].pid) == 0) {
                    valid = false;
                    break;
                }
            }

            // adiciona na fila de alta prioridade se activation_time menor ou igual a tempo atual:
            if (valid) {
                if (processes[i].activation_time <= time) {
                    high_priority_queue[high_priority_queue_size++] = processes[i];
                }
            }
        }

        // verifica quem sai do io e vai para as filas
        Process processes_back_from_io[MAX_QUEUE_SIZE];
        int index_back_from_io = 0;
        for (int i = 0; i < num_items_io_queue; i++) {
            if (io_queue[i].io_return <= time && io_queue[i].io_concluded[io_queue[i].index_io_exec] == false) {
                io_queue[i].io_concluded[io_queue[i].index_io_exec] = true;
                io_queue[i].status = 0;
                processes_back_from_io[index_back_from_io++] = io_queue[i];
            }
        }

        // ordernar por tempo de retorno do io
        qsort(processes_back_from_io, index_back_from_io, sizeof(Process), compareByReturnTime);

        // adiciona processos nas filas de acordo com tipo de io
        for (int i = 0; i < index_back_from_io; i++) {
            if (processes_back_from_io[i].io_type[processes_back_from_io[i].index_io_exec] == 1) {
                low_priority_queue[low_priority_queue_size] = processes_back_from_io[i];
                low_priority_queue[low_priority_queue_size].index_io_exec++;
                low_priority_queue[low_priority_queue_size++].priority = 0;
            } else {
                high_priority_queue[high_priority_queue_size] = processes_back_from_io[i];
                high_priority_queue[high_priority_queue_size].index_io_exec++;
                high_priority_queue[high_priority_queue_size++].priority = 1;
            }
        }

        // OUTPUT DO INTERVALO ATUAL:

        printf("[Ciclo de execução: %d u.t. -> %d u.t.]\n", start_time, time);
        // printar fila após ciclo de execução:
        printf("------------FILA DE ALTA PRIORIDADE------------\n");
        for (int i = 0; i < high_priority_queue_size; i++) {
            if (i < item_high_queue) {
                printf("[-%s-] | ", high_priority_queue[i].pid);
            } else {
                printf("%s | ", high_priority_queue[i].pid);
            }
        }
        printf("\n------------FILA DE BAIXA PRIORIDADE------------\n");
        for (int i = 0; i < low_priority_queue_size; i++) {
            if (i < item_lower_queue) {
                printf("[-%s-] | ", low_priority_queue[i].pid);
            } else {
                printf("%s | ", low_priority_queue[i].pid);
            }
        }
        printf("\n-------------------FILA DE IO-------------------\n");
        char tipo_io[20];
        for (int i = 0; i < num_items_io_queue; i++) {
            if (io_queue[i].io_type[io_queue[i].index_io_exec] == 1) {
                strcpy(tipo_io, disco);
            } else if (io_queue[i].io_type[io_queue[i].index_io_exec] == 2) {
                strcpy(tipo_io, fita);
            } else {
                strcpy(tipo_io, impressora);
            }
            printf("%s -> [%s] - retorna no tempo %d\n", io_queue[i].pid, tipo_io, io_queue[i].io_return);
        }
        printf("\n\n");
    }
}

int main() {
    int num_processes = 3;
    int io_types1[] = {DISCO, IMPRESSORA};
    int io_times1[] = {6, 14};
    int io_types2[] = {DISCO, FITA, IMPRESSORA}; // Disco, Fita e Impressora
    int io_times2[] = {2, 5, 9};
    int io_types3[] = {IMPRESSORA, FITA}; // Impressora e Fita
    int io_times3[] = {4, 6};

    Process processes[num_processes];
    processes[0] = initializeProcess("P1", 0, 15, 2, io_types1, io_times1);
    processes[1] = initializeProcess("P2", 5, 10, 3, io_types2, io_times2);
    processes[2] = initializeProcess("P3", 15, 20, 2, io_types3, io_times3);

    Process *finished_processes = roundRobinScheduler(processes, num_processes);

    for (int i = 0; i < num_processes; i++) {
        printf("Turnaround do processo %s: %d\n", finished_processes[i].pid, finished_processes[i].end_time - finished_processes[i].activation_time);
    }

    // limpar memoria usada para retorno do array de structs
    free(finished_processes);

    return 0;
}
