#include <unity.h>
#include <fstream>
#include <string>

static std::string readFile(const std::string& path) {
  std::ifstream ifs(path);
  return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

void test_dispatch_password_defaults() {
  auto cfg = readFile("variants/t1000-e/platformio.ini");
  TEST_ASSERT_TRUE_MESSAGE(cfg.find("ROOM_PASSWORD='\"pagerUser\"'") != std::string::npos,
                           "ROOM_PASSWORD pagerUser missing in t1000e dispatch env");
  TEST_ASSERT_TRUE_MESSAGE(cfg.find("ADMIN_PASSWORD='\"pagerAdmin\"'") != std::string::npos,
                           "ADMIN_PASSWORD pagerAdmin missing in t1000e dispatch env");
}

void test_dispatch_heartbeat_default() {
  auto hdr = readFile("examples/simple_room_server/MyMesh.h");
  TEST_ASSERT_TRUE_MESSAGE(hdr.find("#define DISPATCH_HEARTBEAT_INTERVAL_MS 60000") != std::string::npos,
                           "Default heartbeat interval should remain 60000 ms");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_dispatch_password_defaults);
  RUN_TEST(test_dispatch_heartbeat_default);
  return UNITY_END();
}
