#pragma once
#include <iostream>
#include <vector>

/*临时构造的dict类，用于发送SPD1数据组包
    github.com/danielpaulus/quicktime_video_hack/screencapture/coremedia/dict.go

    dict格式：
    {
        "Valeria" : true  //false为基座接口(APP播放视频会有不同的协议)，      true为v模式(全程录制屏幕)，时间会显示9:41，信号、电量都会显示满格
        "HEVCDecoderSupports444" : true
        "DisplaySize" :
            {
                "Width"  : 1920
                "Height" : 1200
            }
    }
    bool bValeria = false;  //1为软件捕捉  0为基座接口, 默认0
    if (SCREEN_MODE_SOFTWARE_CAPTURE == gConfig.nScreenMode)
    {
        bValeria = true;
    }
    dict _dict;
    _dict.add_item_uint("Width", 1920);
    _dict.add_item_uint("Height", 1200);
    dict _dict2;
    _dict2.add_item_bool("Valeria", bValeria);
    _dict2.add_item_bool("HEVCDecoderSupports444", true);
    _dict2.add_item_dict("DisplaySize", _dict);
*/

class gcdict
{
public:
    gcdict();
    ~gcdict();
public:
    void add_item_dict(const char* key, const gcdict& item);
    void add_item_uint(const char* key, uint64_t val);
    void add_item_bool(const char* key, bool val);
public:
    std::vector<unsigned char> vec_data;
};
