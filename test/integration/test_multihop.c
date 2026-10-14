#include "unity.h"
#include "network_layer.h"
#include "mesh_frame.h"

void setUp(void) {}
void tearDown(void) {}

static void assert_counters(const network_layer_t *node,
                            uint32_t originated, uint32_t forwarded, uint32_t delivered);

static void init_node(network_layer_t *node, uint8_t address)
{
    const network_layer_config_t config = {.local_addr = address, .default_ttl = 4};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(node, &config));
}

static void wire_roundtrip(const mesh_frame_t *input, mesh_frame_t *output)
{
    uint8_t wire[MESH_FRAME_MAX_WIRE_LEN];
    size_t length = 0;
    TEST_ASSERT_EQUAL(MESH_FRAME_OK, mesh_frame_pack(input, wire, sizeof(wire), &length));
    TEST_ASSERT_EQUAL(MESH_FRAME_OK, mesh_frame_unpack(wire, length, output));
    TEST_ASSERT_EQUAL(input->frame_type, output->frame_type);
    TEST_ASSERT_EQUAL_UINT8(input->flags, output->flags);
    TEST_ASSERT_EQUAL_UINT8(input->src_addr, output->src_addr);
    TEST_ASSERT_EQUAL_UINT8(input->dst_addr, output->dst_addr);
    TEST_ASSERT_EQUAL_UINT8(input->prev_hop, output->prev_hop);
    TEST_ASSERT_EQUAL_UINT8(input->next_hop, output->next_hop);
    TEST_ASSERT_EQUAL_UINT16(input->msg_id, output->msg_id);
    TEST_ASSERT_EQUAL_UINT8(input->ttl, output->ttl);
    TEST_ASSERT_EQUAL_UINT8(input->hop_count, output->hop_count);
    TEST_ASSERT_EQUAL_UINT8(input->payload_len, output->payload_len);
    if (input->payload_len > 0) {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(input->payload, output->payload, input->payload_len);
    }
}

static void assert_counters(const network_layer_t *node,
                            uint32_t originated, uint32_t forwarded, uint32_t delivered)
{
    network_layer_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_get_stats(node, &stats));
    TEST_ASSERT_EQUAL_UINT32(originated, stats.originated);
    TEST_ASSERT_EQUAL_UINT32(forwarded, stats.forwarded);
    TEST_ASSERT_EQUAL_UINT32(delivered, stats.delivered_local);
    TEST_ASSERT_EQUAL_UINT32(0, stats.dropped_no_route);
    TEST_ASSERT_EQUAL_UINT32(0, stats.dropped_ttl);
    TEST_ASSERT_EQUAL_UINT32(0, stats.dropped_not_for_hop);
}

void test_IS01_sensor_bridge_concentrator_preserves_message_over_wire_format(void)
{
    network_layer_t sensor, bridge, concentrator;
    init_node(&sensor, 1);
    init_node(&bridge, 2);
    init_node(&concentrator, 3);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 2, 2, 1000, -30, 10, 1000));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 3, 1, 1000, -15, 14, 1000));
    const uint8_t payload[] = {0x10, 0x20, 0x30, 0x40};
    mesh_frame_t tx, received_at_bridge, received_at_concentrator;
    /* S0: fresh contexts and routes; no message has been processed. */
    assert_counters(&sensor, 0, 0, 0);
    assert_counters(&bridge, 0, 0, 0);
    assert_counters(&concentrator, 0, 0, 0);
    /* T1 -> S1: originate the message at the sensor. */
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(
        &sensor, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, sizeof(payload), &tx));
    TEST_ASSERT_EQUAL_UINT8(2, tx.next_hop);
    TEST_ASSERT_EQUAL_UINT8(4, tx.ttl);
    TEST_ASSERT_EQUAL_UINT8(0, tx.hop_count);
    TEST_ASSERT_EQUAL_UINT8(1, tx.prev_hop);
    TEST_ASSERT_EQUAL_UINT8(1, tx.src_addr);
    TEST_ASSERT_EQUAL_UINT8(3, tx.dst_addr);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), tx.payload_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, tx.payload, sizeof(payload));
    assert_counters(&sensor, 1, 0, 0);
    /* T2 -> S2: encode/decode preserves all fields at the bridge. */
    wire_roundtrip(&tx, &received_at_bridge);
    assert_counters(&bridge, 0, 0, 0);
    /* T3 -> S3: prepare the next software hop, without radio transmission. */
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&bridge, &received_at_bridge));
    TEST_ASSERT_EQUAL_UINT8(3, received_at_bridge.ttl);
    TEST_ASSERT_EQUAL_UINT8(1, received_at_bridge.hop_count);
    TEST_ASSERT_EQUAL_UINT8(2, received_at_bridge.prev_hop);
    TEST_ASSERT_EQUAL_UINT8(3, received_at_bridge.next_hop);
    TEST_ASSERT_EQUAL(tx.frame_type, received_at_bridge.frame_type);
    TEST_ASSERT_EQUAL_UINT8(tx.flags, received_at_bridge.flags);
    TEST_ASSERT_EQUAL_UINT8(tx.src_addr, received_at_bridge.src_addr);
    TEST_ASSERT_EQUAL_UINT8(tx.dst_addr, received_at_bridge.dst_addr);
    TEST_ASSERT_EQUAL_UINT16(tx.msg_id, received_at_bridge.msg_id);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), received_at_bridge.payload_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, received_at_bridge.payload, sizeof(payload));
    assert_counters(&bridge, 0, 1, 0);
    /* T4 -> S4: second encode/decode preserves the prepared frame. */
    wire_roundtrip(&received_at_bridge, &received_at_concentrator);
    assert_counters(&concentrator, 0, 0, 0);
    const mesh_frame_t before_delivery = received_at_concentrator;
    /* T5 -> S5: local delivery preserves the forwarding fields. */
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&concentrator, &received_at_concentrator));
    TEST_ASSERT_EQUAL_UINT8(before_delivery.prev_hop, received_at_concentrator.prev_hop);
    TEST_ASSERT_EQUAL_UINT8(before_delivery.next_hop, received_at_concentrator.next_hop);
    TEST_ASSERT_EQUAL_UINT8(before_delivery.ttl, received_at_concentrator.ttl);
    TEST_ASSERT_EQUAL_UINT8(before_delivery.hop_count, received_at_concentrator.hop_count);
    TEST_ASSERT_TRUE(network_layer_is_local_delivery(&concentrator, &received_at_concentrator));
    TEST_ASSERT_EQUAL(tx.frame_type, received_at_concentrator.frame_type);
    TEST_ASSERT_EQUAL_UINT8(tx.flags, received_at_concentrator.flags);
    TEST_ASSERT_EQUAL_UINT8(1, received_at_concentrator.src_addr);
    TEST_ASSERT_EQUAL_UINT8(3, received_at_concentrator.dst_addr);
    TEST_ASSERT_EQUAL_UINT8(2, received_at_concentrator.prev_hop);
    TEST_ASSERT_EQUAL_UINT8(3, received_at_concentrator.next_hop);
    TEST_ASSERT_EQUAL_UINT16(tx.msg_id, received_at_concentrator.msg_id);
    TEST_ASSERT_EQUAL_UINT8(3, received_at_concentrator.ttl);
    TEST_ASSERT_EQUAL_UINT8(1, received_at_concentrator.hop_count);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), received_at_concentrator.payload_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, received_at_concentrator.payload, sizeof(payload));
    assert_counters(&sensor, 1, 0, 0);
    assert_counters(&bridge, 0, 1, 0);
    assert_counters(&concentrator, 0, 0, 1);
}

/* One parameterized integration case: empty/max payload and final TTL=0 delivery. */
void test_IS02_payload_boundaries_and_last_hop_preserve_confirmed_messages(void)
{
    const size_t lengths[] = {0, MESH_FRAME_MAX_PAYLOAD_LEN};
    uint8_t payload[MESH_FRAME_MAX_PAYLOAD_LEN];
    for (size_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i ^ 0xA5);
    for (size_t n = 0; n < sizeof(lengths) / sizeof(lengths[0]); ++n) {
        network_layer_t sensor, bridge, destination;
        const network_layer_config_t config = {.local_addr = 1, .default_ttl = 1};
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&sensor, &config));
        init_node(&bridge, 2);
        init_node(&destination, 3);
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 2, 2, 1000, -30, 10, 0));
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 3, 1, 1000, -15, 14, 0));
        sensor.next_msg_id = UINT16_MAX;
        mesh_frame_t tx, rx;
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(&sensor, 3,
            MESH_FRAME_TYPE_DATA_CONFIRMED, lengths[n] ? payload : NULL, lengths[n], &tx));
        TEST_ASSERT_EQUAL_UINT16(1, tx.msg_id);
        wire_roundtrip(&tx, &rx);
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&bridge, &rx));
        TEST_ASSERT_EQUAL_UINT8(0, rx.ttl);
        TEST_ASSERT_EQUAL_UINT8(1, rx.hop_count);
        TEST_ASSERT_EQUAL_UINT8(2, rx.prev_hop);
        TEST_ASSERT_EQUAL_UINT8(3, rx.next_hop);
        TEST_ASSERT_EQUAL(tx.frame_type, rx.frame_type);
        TEST_ASSERT_EQUAL_UINT16(tx.msg_id, rx.msg_id);
        TEST_ASSERT_EQUAL_UINT8(tx.src_addr, rx.src_addr);
        TEST_ASSERT_EQUAL_UINT8(tx.dst_addr, rx.dst_addr);
        TEST_ASSERT_EQUAL_UINT8(lengths[n], rx.payload_len);
        if (lengths[n]) TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, rx.payload, lengths[n]);
        mesh_frame_t delivered;
        wire_roundtrip(&rx, &delivered);
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&destination, &delivered));
        TEST_ASSERT_TRUE(network_layer_is_local_delivery(&destination, &delivered));
        TEST_ASSERT_EQUAL_UINT8(0, delivered.ttl);
        TEST_ASSERT_EQUAL_UINT8(1, delivered.hop_count);
        assert_counters(&sensor, 1, 0, 0);
        assert_counters(&bridge, 0, 1, 0);
        assert_counters(&destination, 0, 0, 1);
    }
}

/* Serialized frames rejected at the bridge must not be modified or forwarded. */
void test_IS03_decoded_frames_reject_wrong_hop_expired_ttl_and_missing_route(void)
{
    const esp_err_t expected[] = {NETWORK_LAYER_ERR_NOT_FOR_LOCAL_HOP,
        NETWORK_LAYER_ERR_TTL_EXPIRED, NETWORK_LAYER_ERR_NO_ROUTE, ESP_ERR_INVALID_STATE};
    for (size_t scenario = 0; scenario < sizeof(expected) / sizeof(expected[0]); ++scenario) {
        network_layer_t sensor, bridge;
        init_node(&sensor, 1);
        init_node(&bridge, 2);
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 2, 2, 1000, -30, 10, 0));
        if (scenario != 2) TEST_ASSERT_EQUAL(ESP_OK,
            network_layer_upsert_route(&bridge, 3, 3, 1, 1000, -15, 14, 0));
        const uint8_t payload[] = {0x11, 0x22};
        mesh_frame_t tx, rx;
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(&sensor, 3,
            MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, sizeof(payload), &tx));
        if (scenario == 0) tx.next_hop = 4;
        if (scenario == 1) tx.ttl = 0;
        if (scenario == 3) bridge.initialized = false;
        wire_roundtrip(&tx, &rx);
        TEST_ASSERT_FALSE(network_layer_is_local_delivery(&bridge, &rx));
        TEST_ASSERT_EQUAL(expected[scenario], network_layer_prepare_forward(&bridge, &rx));
        /* Re-encoding rejected input must yield the exact same wire bytes. */
        uint8_t before[MESH_FRAME_MAX_WIRE_LEN], after[MESH_FRAME_MAX_WIRE_LEN];
        size_t before_len, after_len;
        TEST_ASSERT_EQUAL(MESH_FRAME_OK, mesh_frame_pack(&tx, before, sizeof(before), &before_len));
        TEST_ASSERT_EQUAL(MESH_FRAME_OK, mesh_frame_pack(&rx, after, sizeof(after), &after_len));
        TEST_ASSERT_EQUAL(before_len, after_len);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(before, after, before_len);
        network_layer_stats_t stats;
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_get_stats(&bridge, &stats));
        TEST_ASSERT_EQUAL_UINT32(0, stats.originated);
        TEST_ASSERT_EQUAL_UINT32(0, stats.forwarded);
        TEST_ASSERT_EQUAL_UINT32(0, stats.delivered_local);
        TEST_ASSERT_EQUAL_UINT32(scenario == 0, stats.dropped_not_for_hop);
        TEST_ASSERT_EQUAL_UINT32(scenario == 1, stats.dropped_ttl);
        TEST_ASSERT_EQUAL_UINT32(scenario == 2, stats.dropped_no_route);
        assert_counters(&sensor, 1, 0, 0);
    }
}

static void assert_message_uses_route(network_layer_t *sensor, uint8_t next_hop)
{
    network_layer_t bridge, destination;
    init_node(&bridge, next_hop);
    init_node(&destination, 3);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&bridge, 3, 3, 1, 1000, -15, 14, 0));
    const uint8_t payload[] = {0x5A, 0xC3};
    mesh_frame_t tx, rx, delivered;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(sensor, 3,
        MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, sizeof(payload), &tx));
    TEST_ASSERT_EQUAL_UINT8(next_hop, tx.next_hop);
    wire_roundtrip(&tx, &rx);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&bridge, &rx));
    wire_roundtrip(&rx, &delivered);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&destination, &delivered));
    TEST_ASSERT_EQUAL_UINT8(next_hop, delivered.prev_hop);
    TEST_ASSERT_EQUAL_UINT8(3, delivered.next_hop);
    TEST_ASSERT_EQUAL_UINT8(3, delivered.ttl);
    TEST_ASSERT_EQUAL_UINT8(1, delivered.hop_count);
    TEST_ASSERT_EQUAL_UINT8(1, delivered.src_addr);
    TEST_ASSERT_EQUAL_UINT8(3, delivered.dst_addr);
    TEST_ASSERT_EQUAL_UINT16(tx.msg_id, delivered.msg_id);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), delivered.payload_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, delivered.payload, sizeof(payload));
    assert_counters(&bridge, 0, 1, 0);
    assert_counters(&destination, 0, 0, 1);
}

/* Route-table changes must affect actual encoded and delivered messages. */
void test_IS04_route_updates_failover_and_full_table_change_delivered_next_hop(void)
{
    network_layer_t sensor;
    const network_layer_config_t config = {.local_addr = 1, .default_ttl = 0};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_init(&sensor, &config));
    TEST_ASSERT_EQUAL_UINT8(NETWORK_LAYER_DEFAULT_TTL, sensor.config.default_ttl);
    sensor.config.default_ttl = 4;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 2, 1, 1000, -30, 10, 0));
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 4, 2, 1000, -20, 10, 0));
    assert_message_uses_route(&sensor, 2);
    TEST_ASSERT_EQUAL_UINT32(1, network_layer_remove_routes_via_next_hop(&sensor, 2));
    assert_message_uses_route(&sensor, 4);
    /* Fill all remaining slots; the last route is deliberately the worst. */
    for (size_t i = 0; i < NETWORK_LAYER_MAX_ROUTES - 1; ++i)
        TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor,
            (uint8_t)(10 + i), (uint8_t)(30 + i), (uint8_t)(3 + i), 500, -80, 0, 0));
    size_t capacity = 0;
    const network_route_entry_t *routes = network_layer_get_routes(&sensor, &capacity);
    TEST_ASSERT_NOT_NULL(routes);
    TEST_ASSERT_EQUAL_UINT32(NETWORK_LAYER_MAX_ROUTES, capacity);
    for (size_t i = 0; i < capacity; ++i) TEST_ASSERT_TRUE(routes[i].valid);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 5, 1, 900, -20, 10, 0));
    TEST_ASSERT_EQUAL_UINT32(0, network_layer_remove_routes_via_next_hop(&sensor,
        (uint8_t)(30 + NETWORK_LAYER_MAX_ROUTES - 2)));
    assert_message_uses_route(&sensor, 5);
    /* Update existing route: ties are resolved by delivery, RSSI, then SNR. */
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 4, 1, 1000, -50, 0, 1));
    assert_message_uses_route(&sensor, 4);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 5, 1, 1000, -10, 0, 2));
    assert_message_uses_route(&sensor, 5);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 4, 1, 1000, -10, 10, 3));
    assert_message_uses_route(&sensor, 4);
    TEST_ASSERT_EQUAL_UINT32(1, network_layer_remove_routes_via_next_hop(&sensor, 4));
    TEST_ASSERT_EQUAL_UINT32(1, network_layer_remove_routes_via_next_hop(&sensor, 5));
    mesh_frame_t unchanged = {0}, output = unchanged;
    TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_NO_ROUTE, network_layer_build_data(&sensor, 3,
        MESH_FRAME_TYPE_DATA_BEST_EFFORT, NULL, 0, &output));
    TEST_ASSERT_EQUAL_UINT16(unchanged.msg_id, output.msg_id);
    network_layer_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_get_stats(&sensor, &stats));
    TEST_ASSERT_EQUAL_UINT32(6, stats.originated);
    TEST_ASSERT_EQUAL_UINT32(1, stats.dropped_no_route);
    const uint16_t last_id = sensor.next_msg_id;
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_reset_stats(&sensor));
    TEST_ASSERT_EQUAL_UINT16(last_id, sensor.next_msg_id);
    assert_counters(&sensor, 0, 0, 0);
}

/* Transport corruption/size errors are rejected before network processing. */
void test_IS05_wire_corruption_and_buffer_limits_reject_before_delivery(void)
{
    network_layer_t sensor, bridge;
    init_node(&sensor, 1);
    init_node(&bridge, 2);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_upsert_route(&sensor, 3, 2, 2, 1000, -30, 10, 0));
    const uint8_t payload[] = {0x11, 0x22};
    mesh_frame_t tx, decoded = {0};
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(&sensor, 3,
        MESH_FRAME_TYPE_DATA_CONFIRMED, payload, sizeof(payload), &tx));
    uint8_t wire[MESH_FRAME_MAX_WIRE_LEN], damaged[MESH_FRAME_MAX_WIRE_LEN];
    size_t length = 77;
    mesh_frame_result_t result = mesh_frame_pack(&tx, wire, 1, &length);
    TEST_ASSERT_EQUAL(MESH_FRAME_ERR_BUFFER_TOO_SMALL, result);
    TEST_ASSERT_EQUAL_STRING("BUFFER_TOO_SMALL", mesh_frame_result_to_string(result));
    TEST_ASSERT_EQUAL_UINT32(77, length);
    TEST_ASSERT_EQUAL(MESH_FRAME_OK, mesh_frame_pack(&tx, wire, sizeof(wire), &length));
    const mesh_frame_result_t errors[] = {MESH_FRAME_ERR_LENGTH, MESH_FRAME_ERR_MAGIC,
        MESH_FRAME_ERR_PAYLOAD_TOO_LARGE, MESH_FRAME_ERR_LENGTH, MESH_FRAME_ERR_CRC};
    const char *names[] = {"LENGTH", "MAGIC", "PAYLOAD_TOO_LARGE", "LENGTH", "CRC"};
    for (size_t scenario = 0; scenario < sizeof(errors) / sizeof(errors[0]); ++scenario) {
        for (size_t i = 0; i < length; ++i) damaged[i] = wire[i];
        size_t received_len = length;
        if (scenario == 0) received_len = MESH_FRAME_HEADER_LEN - 1;
        if (scenario == 1) damaged[0] ^= 1;
        if (scenario == 2) damaged[11] = MESH_FRAME_MAX_PAYLOAD_LEN + 1;
        if (scenario == 3) --received_len;
        if (scenario == 4) damaged[MESH_FRAME_HEADER_LEN] ^= 1;
        decoded.msg_id = 999;
        result = mesh_frame_unpack(damaged, received_len, &decoded);
        TEST_ASSERT_EQUAL(errors[scenario], result);
        TEST_ASSERT_EQUAL_STRING(names[scenario], mesh_frame_result_to_string(result));
        TEST_ASSERT_EQUAL_UINT16(999, decoded.msg_id);
        assert_counters(&bridge, 0, 0, 0);
    }
    tx.payload_len = MESH_FRAME_MAX_PAYLOAD_LEN + 1;
    TEST_ASSERT_EQUAL(MESH_FRAME_ERR_PAYLOAD_TOO_LARGE, mesh_frame_pack(&tx, wire, sizeof(wire), &length));
    tx.payload_len = sizeof(payload);
    wire_roundtrip(&tx, &decoded);
    TEST_ASSERT_EQUAL_STRING("OK", mesh_frame_result_to_string(MESH_FRAME_OK));
    assert_counters(&sensor, 1, 0, 0);
    assert_counters(&bridge, 0, 0, 0);
}

