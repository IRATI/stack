#ifndef __RL_CPP_UTILS_H__
#define __RL_CPP_UTILS_H__

#include <string>
#include <sstream>

static inline std::string
rina_string_from_components(const std::string &apn, const std::string &api,
                            const std::string &aen, const std::string &aei)
{
#if 1 /* minimized name */
    std::string acc;

    if (aei.size()) {
        acc = std::string("|") + aei + acc;
    }

    if (aen.size()) {
        acc = std::string("|") + aen + acc;
    }

    if (api.size()) {
        acc = std::string("|") + api + acc;
    }

    return apn + acc;
#else /* canonical name */
    if (!apn.size()) {
        return std::string();
    }

    return apn + "/" + api + "/" + aen + "/" + aei;
#endif
}

static inline void
rina_components_from_string(const std::string &str, std::string &apn,
                            std::string &api, std::string &aen,
                            std::string &aei)
{
    std::string *vps[] = {&apn, &api, &aen, &aei};
    std::stringstream ss(str);

    for (int i = 0; i < 4 && std::getline(ss, *(vps[i]), ':'); i++) {
    }
}

#endif /* __RL_CPP_UTILS_H__ */
