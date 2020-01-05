#include "impl-list.hpp"
#include "basic-impls.hpp"
#include "common-cxx.hpp"
#include "misc.hpp"

const test_description all_funcs[] = {
    {"memcpy_stdlib",  memcpy_stdlib,  "C stdlib memcpy", NONE},
    {"memmove_stdlib", memmove_stdlib, "C stdlib memmove", NONE},
    {"repmovsb",       repmovsb,       "inline asm rep movsb", NONE},
    {"vporxmm",        vporxmm,        "vpor xmm", NO_VZ },
    {"vporymm",        vporymm,        "vpor ymm", NO_VZ },
    {"vporzmm",        vporzmm,        "vpor zmm", NO_VZ },
    {"vporxmm_vz",     vporxmm_vz,     "vpor xmm w/ vzeroupper", NONE},
    {"vporymm_vz",     vporymm_vz,     "vpor ymm w/ vzeroupper", NONE},
    {"vporzmm_vz",     vporzmm_vz,     "vpor zmm w/ vzeroupper", NONE},
    {"vporxmm_vz100",  vporxmm_vz100,  "100x vpor lat xmm w/ vzero", NONE},
    {"vporymm_vz100",  vporymm_vz100,  "100x vpor lat ymm w/ vzero", NONE},
    {"vporzmm_vz100",  vporzmm_vz100,  "100x vpor lat zmm w/ vzero", NONE},
    {"vporxmm_tput_vz100",  vporxmm_tput_vz100,  "100x vpor tput xmm w/ vzero", NONE},
    {"vporymm_tput_vz100",  vporymm_tput_vz100,  "100x vpor tput ymm w/ vzero", NONE},
    {"vporzmm_tput_vz100",  vporzmm_tput_vz100,  "100x vpor tput zmm w/ vzero", NONE},
    {"dummy",          dummy,          "empty function", NONE},
};

auto b() -> decltype(get_all().begin()) {
    return get_all().begin();
}

auto e() -> decltype(get_all().end()) {
    return get_all().end();
}

const test_description* get_by_name(const std::string& name) {
    auto it = std::find_if(b(), e(), [&](auto d) { return name == d.name; });
    return it == e() ? nullptr : &*it;
}

std::vector<test_description> get_by_list(const std::string& list) {
    std::vector<test_description> ret;
    for (auto& name : split(list, ",")) {
        auto t = get_by_name(name);
        if (!t) {
            throw std::runtime_error("no test named " + name);
        }
        ret.push_back(*t);
    }
    return ret;
}

const std::vector<test_description>& get_all() {
    static std::vector<test_description> all =
            std::vector<test_description>(all_funcs, all_funcs + COUNT_OF(all_funcs));
    return all;
}
