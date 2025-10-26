#pragma once
#include <iostream>
#include <vector>

/*��ʱ�����dict�࣬���ڷ���SPD1�������
    github.com/danielpaulus/quicktime_video_hack/screencapture/coremedia/dict.go

    dict��ʽ��
    {
        "Valeria" : true  //falseΪ�����ӿ�(APP������Ƶ���в�ͬ��Э��)��      trueΪvģʽ(ȫ��¼����Ļ)��ʱ�����ʾ9:41���źš�����������ʾ����
        "HEVCDecoderSupports444" : true
        "DisplaySize" :
            {
                "Width"  : 1920
                "Height" : 1200
            }
    }
    bool bValeria = false;  //1Ϊ�����׽  0Ϊ�����ӿ�, Ĭ��0
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
