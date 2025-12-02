void wait_turn(int id) {
    pthread_mutex_lock(&queue_lock);
    int my_ticket = next_ticket++;                   // Get a ticket
    while (my_ticket != serving_ticket)              // Wait for turn
        pthread_cond_wait(&queue_cond, &queue_lock);
    pthread_mutex_unlock(&queue_lock);
}