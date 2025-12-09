#include <unity.h>
#include <string>
#include <vector>
#include <sstream>

namespace pager {

struct ParsedPager {
  bool is_pager = false;
  uint8_t page_num = 0;
  uint8_t page_total = 0;
  std::vector<std::string> groups;
  std::string prio;
  std::string body;
};

static std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    if (!item.empty()) out.push_back(item);
  }
  return out;
}

ParsedPager parse(const std::string& text) {
  ParsedPager r;
  if (text.empty()) return r;

  std::stringstream ss(text);
  std::string token;
  while (ss >> token) {
    if (!r.is_pager && (token.rfind("[GRP:", 0) == 0 || token.rfind("[PRIO:", 0) == 0)) {
      r.is_pager = true;  // consider tagged messages as pager/dispatch traffic
    }
    if (token.find('/') != std::string::npos && token[0] >= '0' && token[0] <= '9') {
      auto slash = token.find('/');
      r.page_num = std::stoi(token.substr(0, slash));
      r.page_total = std::stoi(token.substr(slash + 1));
      r.is_pager = true;
    } else if (token.rfind("[GRP:", 0) == 0 && token.back() == ']') {
      auto inner = token.substr(5, token.size() - 6);  // strip [GRP: and ]
      r.groups = split(inner, ',');
    } else if (token.rfind("[PRIO:", 0) == 0 && token.back() == ']') {
      r.prio = token.substr(6, token.size() - 7);
    } else {
      if (r.is_pager) {
        // First non-tag token begins the body; rebuild the rest
        std::string remainder;
        std::getline(ss, remainder);
        r.body = token + remainder;
        if (!r.body.empty() && r.body[0] == ' ') r.body.erase(0, 1);
      }
      break;
    }
  }
  return r;
}

}  // namespace pager

void test_parses_example() {
  const std::string msg = "1/1 [GRP:A,C] [PRIO:D] EMS 2030 Benson Road, Alpha lift assist. Respond Fire 2";
  auto p = pager::parse(msg);
  TEST_ASSERT_TRUE(p.is_pager);
  TEST_ASSERT_EQUAL_UINT8(1, p.page_num);
  TEST_ASSERT_EQUAL_UINT8(1, p.page_total);
  TEST_ASSERT_EQUAL(2, (int)p.groups.size());
  TEST_ASSERT_EQUAL_STRING("A", p.groups[0].c_str());
  TEST_ASSERT_EQUAL_STRING("C", p.groups[1].c_str());
  TEST_ASSERT_EQUAL_STRING("D", p.prio.c_str());
  TEST_ASSERT_TRUE(p.body.find("EMS 2030 Benson Road") == 0);
  TEST_ASSERT_NOT_EQUAL(std::string::npos, p.body.find("Respond Fire 2"));
}

void test_non_pager_pass_through() {
  const std::string msg = "Hello world";
  auto p = pager::parse(msg);
  TEST_ASSERT_FALSE(p.is_pager);
  TEST_ASSERT_EQUAL_UINT8(0, p.page_num);
  TEST_ASSERT_EQUAL_UINT8(0, p.page_total);
  TEST_ASSERT_TRUE(p.groups.empty());
  TEST_ASSERT_TRUE(p.prio.empty());
  TEST_ASSERT_TRUE(p.body.empty());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parses_example);
  RUN_TEST(test_non_pager_pass_through);
  return UNITY_END();
}
