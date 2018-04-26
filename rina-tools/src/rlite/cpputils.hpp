#ifndef __RL_CPP_UTILS_H__
#define __RL_CPP_UTILS_H__

#include <string>
#include <sstream>
#include <memory>
#include <list>

static inline std::list<std::string>
strsplit(const std::string &s, char delim)
{
    std::stringstream ss(s);
    std::list<std::string> ret;
    std::string item;

    while (std::getline(ss, item, delim)) {
        ret.push_back(std::move(item));
    }

    return ret;
}

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

    return apn + "|" + api + "|" + aen + "|" + aei;
#endif
}

static inline void
rina_components_from_string(const std::string &str, std::string &apn,
                            std::string &api, std::string &aen,
                            std::string &aei)
{
    std::string *vps[] = {&apn, &api, &aen, &aei};
    std::stringstream ss(str);

    for (int i = 0; i < 4 && std::getline(ss, *(vps[i]), '|'); i++) {
    }
}

/* This version of make_unique() does not handle arrays. */
template <typename T, typename... Args>
std::unique_ptr<T>
make_unique(Args &&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#define RL_NONCOPIABLE(_CLA)                                                   \
    _CLA(const _CLA &) = delete;                                               \
    _CLA &operator=(const _CLA &) = delete

#define RL_NODEFAULT_NONCOPIABLE(_CLA)                                         \
    _CLA() = delete;                                                           \
    RL_NONCOPIABLE(_CLA)

#define RL_COPIABLE(_CLA)                                                      \
    _CLA(const _CLA &) = default;                                              \
    _CLA &operator=(const _CLA &) = default

#define RL_MOVABLE(_CLA)                                                       \
    _CLA(_CLA &&) = default;                                                   \
    _CLA &operator=(_CLA &&) = default

#define RL_NONMOVABLE(_CLA)                                                    \
    _CLA(_CLA &&) = delete;                                                    \
    _CLA &operator=(_CLA &&) = delete

#define RL_COPIABLE_MOVABLE(_CLA)                                              \
    RL_COPIABLE(_CLA);                                                         \
    RL_MOVABLE(_CLA)

#endif /* __RL_CPP_UTILS_H__ */
