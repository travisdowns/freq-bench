#include "impl-list.hpp"
#include "basic-impls.hpp"
#include "common-cxx.hpp"
#include "misc.hpp"

const test_description all_funcs[] = {
    {"memcpy_stdlib",  memcpy_stdlib,      0, "C stdlib memcpy", NONE},
    {"memmove_stdlib", memmove_stdlib,     0, "C stdlib memmove", NONE},
    {"repmovsb",       repmovsb,           0, "inline asm rep movsb", NONE},
    {"vporxmm",        vporxmm,            0, "vpor xmm", NO_VZ },
    {"vporymm",        vporymm,            0, "vpor ymm", NO_VZ },
    {"vporzmm",        vporzmm,            0, "vpor zmm", NO_VZ },
    {"vporxmm_vz",     vporxmm_vz,         0, "vpor xmm w/ vzeroupper", NONE},
    {"vporymm_vz",     vporymm_vz,         0, "vpor ymm w/ vzeroupper", NONE},
    {"vporzmm_vz",     vporzmm_vz,         0, "vpor zmm w/ vzeroupper", NONE},
    {"vporxmm_vz100",  vporxmm_vz100, 150000, "100x vpor xmm w/ vzero", NONE},
    {"vporymm_vz100",  vporymm_vz100, 150000, "100x vpor ymm w/ vzero", NONE},
    {"vporzmm_vz100",  vporzmm_vz100, 150000, "100x vpor zmm w/ vzero", NONE},
    {"dummy",          dummy,              0, "empty function", NONE},
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
