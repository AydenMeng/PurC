#include "private/ejson.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


TEST(ejson, create_reset_destroy)
{
    struct pcejson* parser = pcejson_create(10, 1);
    ASSERT_NE(parser, nullptr);
    ASSERT_EQ(parser->state, ejson_init_state);
    ASSERT_EQ(parser->depth, 10);
    ASSERT_EQ(parser->flags, 1);

    parser->state = ejson_finished_state;
    ASSERT_NE(parser->state, ejson_init_state);
    ASSERT_EQ(parser->state, ejson_finished_state);

    pcejson_reset(parser, 20, 2);
    ASSERT_EQ(parser->state, ejson_init_state);
    ASSERT_EQ(parser->depth, 20);
    ASSERT_EQ(parser->flags, 2);

    pcejson_destroy(parser);
}

TEST(ejson_token, create_destroy)
{
    struct pcejson_token* token = pcejson_token_new(ejson_token_start_object, -1, 0);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, ejson_token_start_object);
    ASSERT_EQ(token->rws, nullptr);

    pcejson_token_destroy(token);
}

TEST(ejson_token, next_token)
{
    char json[] = "{ \"key\" : \"value\" }";

    purc_rwstream_t rws = purc_rwstream_new_from_mem(json, strlen(json));
    struct pcejson* parser = pcejson_create(10, 1);

    struct pcejson_token* token = pcejson_next_token(parser, rws);
    ASSERT_NE(token, nullptr);
    ASSERT_EQ(token->type, ejson_token_start_object);

    pcejson_token_destroy(token);
    pcejson_destroy(parser);
    purc_rwstream_destroy(rws);
}

TEST(ejson_stack, new_destory)
{
    pcejson_stack* stack = pcejson_stack_new(10);
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->capacity, 32);
    ASSERT_EQ(stack->last, -1);

    pcejson_stack_destroy(stack);
}

TEST(ejson_stack, push_pop)
{
    pcejson_stack* stack = pcejson_stack_new(10);
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->capacity, 32);
    ASSERT_EQ(stack->last, -1);

    bool empty = pcejson_stack_is_empty(stack);
    ASSERT_EQ(empty, true);

    pcejson_stack_push(stack, 1);
    ASSERT_EQ(stack->last, 0);

    uint8_t v = pcejson_stack_last(stack);
    ASSERT_EQ(v, 1);

    pcejson_stack_pop(stack);
    ASSERT_EQ(stack->last, -1);

    pcejson_stack_push(stack, 1);
    pcejson_stack_push(stack, 2);
    pcejson_stack_push(stack, 3);
    pcejson_stack_push(stack, 4);
    pcejson_stack_push(stack, 5);
    pcejson_stack_push(stack, 6);
    pcejson_stack_push(stack, 7);
    pcejson_stack_push(stack, 8);
    pcejson_stack_push(stack, 9);
    pcejson_stack_push(stack, 10);
    pcejson_stack_push(stack, 11);
    pcejson_stack_push(stack, 12);
    pcejson_stack_push(stack, 13);
    pcejson_stack_push(stack, 14);
    pcejson_stack_push(stack, 15);
    pcejson_stack_push(stack, 16);
    pcejson_stack_push(stack, 17);
    pcejson_stack_push(stack, 18);
    pcejson_stack_push(stack, 19);
    pcejson_stack_push(stack, 20);
    pcejson_stack_push(stack, 21);
    pcejson_stack_push(stack, 22);
    pcejson_stack_push(stack, 23);
    pcejson_stack_push(stack, 24);
    pcejson_stack_push(stack, 25);
    pcejson_stack_push(stack, 26);
    pcejson_stack_push(stack, 27);
    pcejson_stack_push(stack, 28);
    pcejson_stack_push(stack, 29);
    pcejson_stack_push(stack, 30);
    pcejson_stack_push(stack, 31);
    pcejson_stack_push(stack, 32);
    pcejson_stack_push(stack, 33);
    ASSERT_EQ(stack->last, 32);
    ASSERT_GT(stack->capacity, 32);

    pcejson_stack_push(stack, 34);
    ASSERT_EQ(stack->last, 33);
    ASSERT_GT(stack->capacity, 32);

    pcejson_stack_pop(stack);
    ASSERT_EQ(stack->last, 32);

    v = pcejson_stack_last(stack);
    ASSERT_EQ(v, 33);

    pcejson_stack_pop(stack);
    pcejson_stack_pop(stack);
    pcejson_stack_pop(stack);
    pcejson_stack_pop(stack);
    pcejson_stack_pop(stack);
    ASSERT_EQ(stack->last, 27);
    v = pcejson_stack_last(stack);
    ASSERT_EQ(v, 28);

    pcejson_stack_destroy(stack);
}