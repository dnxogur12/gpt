#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t resource_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resource_B = PTHREAD_MUTEX_INITIALIZER;

// 각 스레드의 상태를 기록 (0: 대기/종료, 1: 실행 중, 2: 작업 완료)
int thread_status[2] = {0, 0};
pthread_t t1, t2;

// [탐지 및 복구 스레드]
void* detector_routine(void* arg) {
    printf("[Monitor] 탐지 스레드 가동 시작...\n");
    
    while (1) {
        sleep(3); // 3초마다 상태 감시

        // 두 스레드가 모두 '실행 중(1)' 상태인데 3초 뒤에도 그대로라면 데드락으로 판단
        if (thread_status[0] == 1 && thread_status[1] == 1) {
            printf("\n⚠️  [탐지] 데드락(Deadlock) 발생 감지!");
            printf("\n♻️  [복구] Thread 1을 강제 종료하여 자원을 회수합니다...\n");
            
            pthread_cancel(t1); // 복구: 스레드 1 강제 종료
            
            // 실제 환경에서는 뮤텍스를 안전하게 unlock 해주는 과정이 필요합니다.
            pthread_mutex_unlock(&resource_A); 
            
            printf("[완료] 시스템이 다시 정상 작동합니다.\n\n");
            break; 
        }
        
        // 모든 스레드가 작업을 마쳤다면 감시 종료
        if (thread_status[0] == 2 && thread_status[1] == 2) break;
    }
    return NULL;
}

void* thread_1_routine(void* arg) {
    thread_status[0] = 1; // 상태 업데이트
    pthread_mutex_lock(&resource_A);
    printf("Thread 1: Locked A. Waiting for B...\n");
    sleep(1);
    
    pthread_mutex_lock(&resource_B);
    printf("Thread 1: Locked both A and B!\n");
    
    thread_status[0] = 2; // 작업 완료
    pthread_mutex_unlock(&resource_B);
    pthread_mutex_unlock(&resource_A);
    return NULL;
}

void* thread_2_routine(void* arg) {
    thread_status[1] = 1; // 상태 업데이트
    pthread_mutex_lock(&resource_B);
    printf("Thread 2: Locked B. Waiting for A...\n");
    sleep(1);
    
    pthread_mutex_lock(&resource_A);
    printf("Thread 2: Locked both B and A!\n");
    
    thread_status[1] = 2; // 작업 완료
    pthread_mutex_unlock(&resource_A);
    pthread_mutex_unlock(&resource_B);
    return NULL;
}

int main() {
    pthread_t detector;

    pthread_create(&t1, NULL, thread_1_routine, NULL);
    pthread_create(&t2, NULL, thread_2_routine, NULL);
    pthread_create(&detector, NULL, detector_routine, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(detector, NULL);

    printf("Main: 모든 스레드 종료.\n");
    return 0;
}