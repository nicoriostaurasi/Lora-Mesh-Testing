#include "unity.h"
#include "network_layer.h"

static network_layer_t bridge;
static mesh_frame_t frame;

static void assert_frame_equal(const mesh_frame_t *expected, const mesh_frame_t *actual)
{
    TEST_ASSERT_EQUAL(expected->frame_type, actual->frame_type);
    TEST_ASSERT_EQUAL_UINT8(expected->flags, actual->flags);
    TEST_ASSERT_EQUAL_UINT8(expected->src_addr, actual->src_addr);
    TEST_ASSERT_EQUAL_UINT8(expected->dst_addr, actual->dst_addr);
    TEST_ASSERT_EQUAL_UINT8(expected->prev_hop, actual->prev_hop);
    TEST_ASSERT_EQUAL_UINT8(expected->next_hop, actual->next_hop);
    TEST_ASSERT_EQUAL_UINT16(expected->msg_id, actual->msg_id);
    TEST_ASSERT_EQUAL_UINT8(expected->ttl, actual->ttl);
    TEST_ASSERT_EQUAL_UINT8(expected->hop_count, actual->hop_count);
    TEST_ASSERT_EQUAL_UINT8(expected->payload_len, actual->payload_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected->payload, actual->payload, expected->payload_len);
}

static void assert_stats(uint32_t forwarded, uint32_t delivered,
                         uint32_t no_route, uint32_t ttl, uint32_t not_for_hop)
{
    network_layer_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_get_stats(&bridge, &stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.originated);
    TEST_ASSERT_EQUAL_UINT32(forwarded, stats.forwarded);
    TEST_ASSERT_EQUAL_UINT32(delivered, stats.delivered_local);
    TEST_ASSERT_EQUAL_UINT32(no_route, stats.dropped_no_route);
    TEST_ASSERT_EQUAL_UINT32(ttl, stats.dropped_ttl);
    TEST_ASSERT_EQUAL_UINT32(not_for_hop, stats.dropped_not_for_hop);
}

static void add_route(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(
        &bridge, 3, 3, 1, 1000, -15, 14, 1000));
}

void setUp(void)
{
    const network_layer_config_t config = {.local_addr = 2, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &config));
    frame = (mesh_frame_t){
        .frame_type = MESH_FRAME_TYPE_DATA_BEST_EFFORT,
        .flags = 0x05,
        .src_addr = 1, .dst_addr = 3, .prev_hop = 1, .next_hop = 2,
        .msg_id = 99, .ttl = 2, .hop_count = 0,
        .payload_len = 4, .payload = {0x10, 0x20, 0x30, 0x40}
    };
}

void tearDown(void) {}

void test_CP01_null_context_preserves_frame(void)
{
    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_prepare_forward(NULL, &frame));
    assert_frame_equal(&before, &frame);
}

void test_CP02_uninitialized_context_preserves_frame_and_counters(void)
{
    bridge.initialized = false;
    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_prepare_forward(&bridge, &frame));
    assert_frame_equal(&before, &frame);
    assert_stats(0, 0, 0, 0, 0);
}

void test_CP03_null_frame_preserves_counters(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_prepare_forward(&bridge, NULL));
    assert_stats(0, 0, 0, 0, 0);
}

void test_CP04_wrong_next_hop_is_rejected_without_mutation(void)
{
    frame.next_hop = 4;
    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_NOT_FOR_LOCAL_HOP,
                      network_layer_prepare_forward(&bridge, &frame));
    assert_frame_equal(&before, &frame);
    assert_stats(0, 0, 0, 0, 1);
}

void test_CP05_local_delivery_precedes_zero_ttl_and_route_lookup(void)
{
    frame.dst_addr = 2;
    frame.ttl = 0;
    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&bridge, &frame));
    assert_frame_equal(&before, &frame);
    assert_stats(0, 1, 0, 0, 0);
}

void test_CP06_expired_ttl_is_rejected_even_with_route(void)
{
    add_route();
    frame.ttl = 0;
    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_TTL_EXPIRED,
                      network_layer_prepare_forward(&bridge, &frame));
    assert_frame_equal(&before, &frame);
    assert_stats(0, 0, 0, 1, 0);
}

void test_CP07_missing_route_is_rejected_without_mutation(void)
{
    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_NO_ROUTE,
                      network_layer_prepare_forward(&bridge, &frame));
    assert_frame_equal(&before, &frame);
    assert_stats(0, 0, 1, 0, 0);
}

static void assert_successful_forward(uint8_t initial_ttl)
{
    add_route();
    frame.ttl = initial_ttl;
    mesh_frame_t expected = frame;
    expected.ttl = initial_ttl - 1;
    expected.hop_count = 1;
    expected.prev_hop = 2;
    expected.next_hop = 3;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&bridge, &frame));
    assert_frame_equal(&expected, &frame);
    assert_stats(1, 0, 0, 0, 0);
}

void test_CP08_forward_updates_headers_and_preserves_identity_and_payload(void)
{
    assert_successful_forward(2);
}

void test_CP09_ttl_one_allows_last_forward(void)
{
    assert_successful_forward(1);
}
