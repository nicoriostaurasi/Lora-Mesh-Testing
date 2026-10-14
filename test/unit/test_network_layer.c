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
    if (expected->payload_len > 0) {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected->payload, actual->payload, expected->payload_len);
    }
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


static void assert_selected_hop(uint8_t hop)
{
    network_route_entry_t route;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_select_route(&bridge, 3, &route));
    TEST_ASSERT_EQUAL_UINT8(hop, route.next_hop);
}

void test_CP10_init_rejects_null_and_restores_clean_default_state(void)
{
    const network_layer_config_t config = {.local_addr = 2, .default_ttl = 0};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_init(NULL, &config));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_init(&bridge, NULL));
    add_route();
    bridge.stats.forwarded = 5;
    bridge.next_msg_id = 99;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &config));
    TEST_ASSERT_TRUE(bridge.initialized);
    TEST_ASSERT_EQUAL_UINT8(2, bridge.config.local_addr);
    TEST_ASSERT_EQUAL_UINT8(NETWORK_LAYER_DEFAULT_TTL, bridge.config.default_ttl);
    TEST_ASSERT_EQUAL_UINT16(0, bridge.next_msg_id);
    for (size_t i = 0; i < NETWORK_LAYER_MAX_ROUTES; ++i) {
        TEST_ASSERT_FALSE(bridge.routes[i].valid);
    }
    assert_stats(0, 0, 0, 0, 0);
}

void test_CP11_route_validation_and_delivery_boundaries(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
        network_layer_upsert_route(NULL, 3, 3, 1, 1000, -15, 14, 1));
    bridge.initialized = false;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE,
        network_layer_upsert_route(&bridge, 3, 3, 1, 1000, -15, 14, 1));
    bridge.initialized = true;
    const uint16_t invalid[][4] = {{2, 3, 1, 1000}, {3, 2, 1, 1000},
                                  {3, 3, 0, 1000}, {3, 3, 1, 1001}};
    for (size_t i = 0; i < 4; ++i) {
        TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_upsert_route(
            &bridge, invalid[i][0], invalid[i][1], invalid[i][2], invalid[i][3], -15, 14, 1));
    }
    for (size_t i = 0; i < NETWORK_LAYER_MAX_ROUTES; ++i) {
        TEST_ASSERT_FALSE(bridge.routes[i].valid);
    }
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 3, 1, 0, -15, 14, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 4, 1, 1000, -15, 14, 1));

    network_route_entry_t route = {.next_hop = 77};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_select_route(NULL, 3, &route));
    bridge.initialized = false;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_select_route(&bridge, 3, &route));
    bridge.initialized = true;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_select_route(&bridge, 3, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_select_route(&bridge, 2, &route));
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_NO_ROUTE, network_layer_select_route(&bridge, 99, &route));
    TEST_ASSERT_EQUAL_UINT8(77, route.next_hop);

}

void test_CP12_update_and_remove_routes_preserve_other_entries(void)
{
    {

    add_route();
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 4, 3, 1, 1000, -1, 20, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 4, 3, 800, -30, 2, 2));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 3, 2, 900, -20, 5, 2000));
    network_route_entry_t route;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_select_route(&bridge, 3, &route));
    TEST_ASSERT_EQUAL_UINT8(3, route.dst_addr);
    TEST_ASSERT_EQUAL_UINT8(3, route.next_hop);
    TEST_ASSERT_EQUAL_UINT8(2, route.hop_count);
    TEST_ASSERT_EQUAL_UINT16(900, route.delivery_rate_permille);
    TEST_ASSERT_EQUAL_INT16(-20, route.rssi_dbm);
    TEST_ASSERT_EQUAL_INT8(5, route.snr_db);
    TEST_ASSERT_EQUAL_UINT32(2000, route.updated_ms);
    size_t valid = 0;
    for (size_t i = 0; i < NETWORK_LAYER_MAX_ROUTES; ++i) valid += bridge.routes[i].valid;
    TEST_ASSERT_EQUAL_UINT32(3, valid);

    }
    {

    add_route();
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 4, 3, 1, 900, -20, 5, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 5, 2, 900, -20, 5, 1));
    TEST_ASSERT_EQUAL_UINT32(2, network_layer_remove_routes_via_next_hop(&bridge, 3));
    assert_selected_hop(5);
    network_route_entry_t route;
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_NO_ROUTE, network_layer_select_route(&bridge, 4, &route));
    TEST_ASSERT_EQUAL_UINT32(0, network_layer_remove_routes_via_next_hop(&bridge, 3));

    }

}

void test_CP13_route_priority_hops_delivery_rssi_snr_and_tie(void)
{
    {

    const network_layer_config_t config = {.local_addr = 2, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &config));

    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 4, 2, 1000, -1, 20, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 5, 1, 100, -100, -10, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 6, 3, 1000, -1, 20, 1));
    assert_selected_hop(5);

    }
    {

    const network_layer_config_t config = {.local_addr = 2, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &config));

    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 4, 1, 500, -1, 20, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 5, 1, 900, -100, -10, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 6, 1, 100, -1, 20, 1));
    assert_selected_hop(5);

    }
    {

    const network_layer_config_t config = {.local_addr = 2, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &config));

    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 4, 1, 900, -50, 20, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 5, 1, 900, -20, -10, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 6, 1, 900, -100, 20, 1));
    assert_selected_hop(5);

    }
    {

    const network_layer_config_t config = {.local_addr = 2, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &config));

    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 4, 1, 900, -20, 5, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 5, 1, 900, -20, 10, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 6, 1, 900, -20, 0, 1));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 7, 1, 900, -20, 10, 2));
    assert_selected_hop(5);

    }

}

void test_CP14_build_data_zero_one_and_maximum_payload(void)
{
    add_route();
    uint8_t payload[MESH_FRAME_MAX_PAYLOAD_LEN];
    for (size_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i ^ 0xA5);
    const size_t lengths[] = {0, 1, MESH_FRAME_MAX_PAYLOAD_LEN};
    for (size_t i = 0; i < 3; ++i) {
        mesh_frame_type_t type = i == 1 ? MESH_FRAME_TYPE_DATA_CONFIRMED : MESH_FRAME_TYPE_DATA_BEST_EFFORT;
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(
            &bridge, 3, type, i == 0 ? NULL : payload, lengths[i], &frame));
        mesh_frame_t expected = {.frame_type = type, .src_addr = 2, .dst_addr = 3,
            .prev_hop = 2, .next_hop = 3, .msg_id = i + 1, .ttl = 4,
            .payload_len = lengths[i]};
        for (size_t j = 0; j < lengths[i]; ++j) expected.payload[j] = payload[j];
        assert_frame_equal(&expected, &frame);
    }
    network_layer_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_get_stats(&bridge, &stats));
    TEST_ASSERT_EQUAL_UINT32(3, stats.originated);
    TEST_ASSERT_EQUAL_UINT32(0, stats.dropped_no_route);

}

void test_CP15_build_data_errors_and_no_route_have_no_unexpected_side_effects(void)
{
    {

    const mesh_frame_t before = frame;
    uint8_t payload[MESH_FRAME_MAX_PAYLOAD_LEN + 1] = {0};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_build_data(
        NULL, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, 1, &frame));
    bridge.initialized = false;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_build_data(
        &bridge, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, 1, &frame));
    bridge.initialized = true;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_build_data(
        &bridge, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, 1, NULL));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_build_data(
        &bridge, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, sizeof(payload), &frame));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_build_data(
        &bridge, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, NULL, 1, &frame));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_build_data(
        &bridge, 3, MESH_FRAME_TYPE_HELLO, payload, 1, &frame));
    assert_frame_equal(&before, &frame);
    TEST_ASSERT_EQUAL_UINT16(0, bridge.next_msg_id);
    assert_stats(0, 0, 0, 0, 0);

    }
    {

    const network_layer_config_t fresh = {.local_addr = 2, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&bridge, &fresh));

    const mesh_frame_t before = frame;
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_NO_ROUTE, network_layer_build_data(
        &bridge, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, NULL, 0, &frame));
    assert_frame_equal(&before, &frame);
    TEST_ASSERT_EQUAL_UINT16(0, bridge.next_msg_id);
    assert_stats(0, 0, 1, 0, 0);

    }

}

void test_CP16_stats_snapshot_and_reset_preserve_routes_and_message_id(void)
{
    network_layer_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_get_stats(NULL, &stats));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, network_layer_get_stats(&bridge, NULL));
    add_route();
    bridge.stats = (network_layer_stats_t){1, 2, 3, 4, 5, 6};
    bridge.next_msg_id = 99;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_get_stats(&bridge, &stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.originated);
    TEST_ASSERT_EQUAL_UINT32(2, stats.forwarded);
    TEST_ASSERT_EQUAL_UINT32(3, stats.delivered_local);
    TEST_ASSERT_EQUAL_UINT32(4, stats.dropped_no_route);
    TEST_ASSERT_EQUAL_UINT32(5, stats.dropped_ttl);
    TEST_ASSERT_EQUAL_UINT32(6, stats.dropped_not_for_hop);
    stats.forwarded = 55;
    TEST_ASSERT_EQUAL_UINT32(2, bridge.stats.forwarded);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_reset_stats(NULL));
    bridge.initialized = false;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, network_layer_reset_stats(&bridge));
    TEST_ASSERT_EQUAL_UINT32(2, bridge.stats.forwarded);
    bridge.initialized = true;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_reset_stats(&bridge));
    assert_stats(0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_UINT16(99, bridge.next_msg_id);
    assert_selected_hop(3);

}

