#include <stdio.h>
#include <stdlib.h>

typedef struct PCB
{
    struct PCB *PCBNext;
    struct PCB *PCBPrev;
    int process_id;
    int arrived_time;
    int execution_time;
    int remaining_time;
    int exit_time;
} PCB;

PCB *create_pcb(int process_id, int arrived_time, int execution_time)
{
    PCB *new_pcb = malloc(sizeof(PCB));
    new_pcb->process_id = process_id;
    new_pcb->arrived_time = arrived_time;
    new_pcb->execution_time = execution_time;
    new_pcb->remaining_time = execution_time;
    new_pcb->exit_time = 0;
    // 초기에는 PCB의 연결을 NULL로 지정한다.
    new_pcb->PCBNext = NULL;
    new_pcb->PCBPrev = NULL;
    // check time을 설정하기 위해 임의로 만듬
    return new_pcb;
}

void destroy_pcb(PCB *pcb)
{
    free(pcb);
}

typedef struct Queue
{
    PCB *head;
    PCB *tail;
} Queue;

Queue *create_queue()
{
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->head = NULL;
    q->tail = NULL;
    return q;
}

void enqueue(Queue *q, PCB *pcb)
{
    if (q->tail == NULL)
    {
        q->head = pcb;
        q->tail = pcb;
        return;
    }
    // 양방향 연결리스트로 이어준다.
    q->tail->PCBNext = pcb;
    pcb->PCBPrev = q->tail;
    q->tail = pcb;
}

PCB *dequeue(Queue *q)
{
    if (q->head == NULL) // 아무것도 안가르키는 경우
    {
        return NULL;
    }
    PCB *temp = q->head;
    if (temp->PCBNext)
    {
        q->head = temp->PCBNext;
    }
    else // PCB가 하나인 경우
    {
        q->head = NULL;
        q->tail = NULL;
        return temp;
    }
    // PCBPrev는 그냥 둔다.
    temp->PCBNext = NULL;
    return temp;
}

int is_queue_empty(Queue *queue)
{
    return queue->head == NULL;
}

void destroy_queue(Queue *queue)
{
    while (!is_queue_empty(queue))
    {
        PCB *dequeued_pcb = dequeue(queue);
        destroy_pcb(dequeued_pcb);
    }
    free(queue);
}

PCB **read_event_data(const char *file_path, int *num_pcbs)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        printf("Error: 파일을 열 수 없습니다.\n");
        return NULL;
    }

    int arrived_time, process_id, execution_time;
    // 파일에서 시나리오가 몇개 인지 세어본다
    int count = 0;
    while (fscanf(file, "%d %d %d", &arrived_time, &process_id, &execution_time) == 3)
    {
        count++;
    }

    // 시나리오에 해당하는 배열을 만들어 주며 각 요소들은 PCB를 가르키는 포인터다
    PCB **pcb_array = malloc(count * sizeof(PCB *));
    *num_pcbs = count;
    // 파일의 포인터를 시작점으로 초기화
    fseek(file, 0, SEEK_SET);
    // 배열에 데이터를 저장
    for (int i = 0; i < count; i++)
    {
        if (fscanf(file, "%d %d %d", &arrived_time, &process_id, &execution_time) == 3)
        {
            PCB *pcb = create_pcb(process_id, arrived_time, execution_time);
            pcb_array[i] = pcb;
        }
    }

    fclose(file);
    return pcb_array;
}

PCB **create_process(PCB **senario_array, int senario_array_size, int current_time, int *num_arrived)
{
    if (senario_array_size == 0)
    {
        *num_arrived = 0;
        return NULL;
    }
    // 특정시간에 도착한 array들을 반환할 배열을 선언한다.
    PCB **arrived_array = NULL;
    int arrived_count = 0;
    // 호출될 때마다 시나리오를 확인하면서 도착시간을 확인하고 해당 도착시간의 PCB들을 반환한다.
    for (int i = 0; i < senario_array_size;)
    {
        if (senario_array[i]->arrived_time <= current_time)
        {
            arrived_count++;
            arrived_array = realloc(arrived_array, sizeof(PCB *) * arrived_count);
            arrived_array[arrived_count - 1] = senario_array[i];

            // Remove the PCB from senario_array by shifting elements
            for (int j = i; j < senario_array_size - 1; j++)
            {
                senario_array[j] = senario_array[j + 1];
            }
            senario_array_size--;
        }
        else
        {
            i++;
        }
    }

    if (arrived_count == 0)
    {
        free(arrived_array);
        *num_arrived = 0;
        return NULL;
    }

    *num_arrived = arrived_count;
    return arrived_array;
}

void round_robin(Queue *ready_queue, int time_quantum, int current_time)
{
    static int check_time; // 전역변수로 몇번 호출되었는지 체크

    if (is_queue_empty(ready_queue))
    {
        return;
    }

    // 매번 현재 ready_queue의 head를 체크한다.
    PCB *current_pcb = ready_queue->head;
    check_time++;
    int time_slice = (current_pcb->remaining_time > time_quantum) ? time_quantum : current_pcb->remaining_time;
    if (check_time == time_slice) // schedulling start
    {
        check_time = 0;
        if (current_pcb->remaining_time - time_slice > 0)
        {
            current_pcb->remaining_time -= time_slice;
            printf("current_time\tPID\t\tArrival\t\texecution_time\texit_time\tremaining_time\tprev_PID\tnext_PID\n");
            printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", current_time, current_pcb->process_id, current_pcb->arrived_time, current_pcb->execution_time, current_pcb->exit_time, current_pcb->remaining_time,
                   (current_pcb->PCBPrev != NULL) ? current_pcb->PCBPrev->process_id : -1,
                   (current_pcb->PCBNext != NULL) ? current_pcb->PCBNext->process_id : -1);
            printf("스케쥴링 발생!! \n");
            printf("-----------------------------------------------------------------------------------------------------------------\n");
            PCB *temp = current_pcb;
            enqueue(ready_queue, temp);
            dequeue(ready_queue);
        }
        else
        {
            current_pcb->remaining_time -= time_slice;
            current_pcb->exit_time = current_time;
            printf("current_time\tPID\t\tArrival\t\texecution_time\texit_time\tremaining_time\tprev_PID\tnext_PID\n");
            printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", current_time, current_pcb->process_id, current_pcb->arrived_time, current_pcb->execution_time, current_pcb->exit_time, current_pcb->remaining_time,
                   (current_pcb->PCBPrev != NULL) ? current_pcb->PCBPrev->process_id : -1,
                   (current_pcb->PCBNext != NULL) ? current_pcb->PCBNext->process_id : -1);
            printf("스케쥴링 발생!! \n");
            PCB *exit_pcb = dequeue(ready_queue);
            ready_queue->head->PCBPrev = NULL; // 이 부분을 안하면 prev_PID가 0으로 출력. 차라리 -1로 표현되게 NULL로 해주자.
            destroy_pcb(exit_pcb);
            printf("-----------------------------------------------------------------------------------------------------------------\n");
        }
    }
}

int main()
{
    int time_quantum = 3;
    int END_TIME = 40;

    const char *event_file_path = "event.txt";
    int senario_array_size;
    PCB **senario_array = read_event_data(event_file_path, &senario_array_size);
    if (senario_array == NULL)
    {
        printf("Error: Failed to read event data from the file.\n");
        return 0;
    }

    Queue *ready_queue = create_queue();

    int current_time = 0;
    while (current_time <= END_TIME)
    {
        int num_arrived;
        PCB **arrived_array = create_process(senario_array, senario_array_size, current_time, &num_arrived);
        if (arrived_array != NULL)
        {
            for (int i = 0; i < num_arrived; i++)
            {
                enqueue(ready_queue, arrived_array[i]);
            }
            senario_array_size -= num_arrived;
            free(arrived_array);
        }
        round_robin(ready_queue, time_quantum, current_time);
        current_time++;
    }
    // Free senario_array
    for (int i = 0; i < senario_array_size; i++)
    {
        destroy_pcb(senario_array[i]);
    }
    free(senario_array);

    destroy_queue(ready_queue);

    return 0;
}
