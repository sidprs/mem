#include <stdio.h>
#include <stdlib.h>

void rb_init(ring_buffer_t *rb, uint8_t *buffer, size_t capacity);

int rb_push(ring_buffer_t *rb, uint8_t value);
// returns 0 on success, -1 if full

int rb_pop(ring_buffer_t *rb, uint8_t *out);
// returns 0 on success, -1 if empty

int rb_is_empty(const ring_buffer_t *rb);

int rb_is_full(const ring_buffer_t *rb);

size_t rb_size(const ring_buffer_t *rb);


typedef struct{
    uint8_t *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;
} ring_buffer_t;

void rb_init(ring_buffer_t *rb, uint8_t *buffer_, size_t capacity){
    rb->buffer = buffer_;
    rb->capacity = capacity;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
}

int rb_push(ring_buffer_t *rb, uint8_t value){
    if (rb == NULL || rb->buffer == NULL || rb->capacity == 0) {
        return -1;
    }

    if (rb->size == rb->capacity){
        printf("rb is full, overwriting oldest");
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->size--;
    }
    else{
        rb->size++;
    }

    rb->buffer[rb->head] = value;
    rb->head = (rb->head + 1) % rb->capacity;
    return 0;
}

int rb_pop(ring_buffer_t *rb, uint8_t *out){
    if(rb == NULL || rb->capacity == 0 || rb->size == 0){
        return -1;
    }
    *out = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    return 0;
}


int rb_is_empty(const ring_buffer_t *rb){
    return rb->size == 0;
}

int rb_is_full(const ring_buffer_t *rb){
    return rb->size == rb->capacity;
}
size_t rb_size(const ring_buffer_t *rb){
    return rb->size;
}



int main(){
    printf("hello\n");
    return 0;
}