#include <string>

class PlanManager {

  static std::string plan_filename;

public:
  PlanManager();

  static std::string get_plan_filename() {
    return plan_filename;
  }

  static void set_plan_filename(std::string s) {
    plan_filename = s;
  }

};
