/*** 
 * @Author: LYK && 2586356361@qq.com
 * @Date: 2026-05-24 22:44:38
 * @LastEditors: LYK && 2586356361@qq.com
 * @LastEditTime: 2026-05-24 22:46:54
 * @FilePath: /beagle_play/extern/beagle_sender/include/types.hpp
 * @Description: 这里放置发送端的一些类型定义  
 * @
 * @Copyright (c) 2026  All Rights Reserved. 
 */

#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
struct SensorData {
    std::string node{"F1"};
    double light{20.0};
    double temperature{28.5};
    double humidity{60.0};
    int rssi{-70};
};


#endif // TYPES_HPP