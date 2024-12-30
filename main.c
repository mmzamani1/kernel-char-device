#include <stdio.h>
#include <string.h>
#include "./queue.c"

int func();

int main()
{
    char arr[10];

    Queue q;
    initializeQueue(&q);

    // Enqueue elements
    enqueue(&q, 10);
    printQueue(&q);

    enqueue(&q, 20);
    printQueue(&q);

    enqueue(&q, 30);
    printQueue(&q);

    // Peek front element
    printf("Front element: %d\n", peek(&q));

    // Dequeue an element
    dequeue(&q);
    printQueue(&q);

    // Peek front element after dequeue
    printf("Front element after dequeue: %d\n", peek(&q));

    scanf("%s", arr);
    printf("\narr: %s\n", arr);

    return 0;
}
