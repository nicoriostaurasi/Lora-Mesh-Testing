#include "unity.h"
#include "network_layer.h"
#include "mesh_frame.h"

void setUp(void) {}
void tearDown(void) {}

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
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_build_data(
        &sensor, 3, MESH_FRAME_TYPE_DATA_BEST_EFFORT, payload, sizeof(payload), &tx));
    TEST_ASSERT_EQUAL_UINT8(2, tx.next_hop);
    TEST_ASSERT_EQUAL_UINT8(4, tx.ttl);
    TEST_ASSERT_EQUAL_UINT8(0, tx.hop_count);
    wire_roundtrip(&tx, &received_at_bridge);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&bridge, &received_at_bridge));
    wire_roundtrip(&received_at_bridge, &received_at_concentrator);
    TEST_ASSERT_EQUAL(ESP_OK, network_layer_prepare_forward(&concentrator, &received_at_concentrator));
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
