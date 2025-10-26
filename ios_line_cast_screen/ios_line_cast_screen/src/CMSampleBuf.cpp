#include "CMSampleBuf.h"
#include "GCUSB.h"

#define LOG_DEBUG(...)
#define LOG_WARNING(...)

const UINT32 sbuf = 0x73627566;	 //the cmsamplebuf and only content of feed asyns
const UINT32 opts = 0x6F707473;	 //output presentation timestamp?
const UINT32 stia = 0x73746961;	 //sampleTimingInfoArray
const UINT32 sdat = 0x73646174;	 //the nalu
const UINT32 satt = 0x73617474;	 //indexkey dict with only number values, CMSampleBufferGetSampleAttachmentsArray
const UINT32 sary = 0x73617279;	 //some dict with index and one boolean
const UINT32 ssiz = 0x7373697A;	 //samplesize in bytes, size of what is contained in sdat, sample size array i think
const UINT32 nsmp = 0x6E736D70;	 //numsample so you know how many things are in the arrays
const UINT32 cmSampleTimingInfoLength = 3 * sizeof(CMTime);


void CMSampleBuffer::parseStia(const unsigned char* ptr)
{
	int length = *(int*)(ptr + 0);
	int magic = *(int*)(ptr + 4);
	if (magic != stia)
	{
		LOG_WARNING("unknown magic type:(0x%x)\n", magic);
		return;
	}
	int stiaLength = length - 8;
	int numEntries = stiaLength / cmSampleTimingInfoLength;
	int modulus = stiaLength % cmSampleTimingInfoLength;
	if (modulus != 0)
	{
		LOG_WARNING("error parsing stia, too many bytes: %d\n", modulus);
		return;
	}
	ptr += 8;
	this->SampleTimingInfoArray.resize(numEntries);
	CMSampleTimingInfo cmSampleTimingInfo;
	for (int i = 0; i < numEntries; i++)
	{
		memcpy(&cmSampleTimingInfo, ptr + (i * sizeof(CMSampleTimingInfo)), sizeof(CMSampleTimingInfo));
		this->SampleTimingInfoArray[i] = cmSampleTimingInfo;
	}
}

void CMSampleBuffer::parseSampleSizeArray(const unsigned char* ptr)
{
	int length = *(int*)(ptr + 0);
	int magic = *(int*)(ptr + 4);
	if (magic != ssiz)
	{
		LOG_WARNING("unknown magic type:(0x%x)\n", magic);
		return;
	}
	int stiaLength = length - 8;
	int numEntries = stiaLength / 4;
	int modulus = stiaLength % 4;
	if (modulus != 0)
	{
		LOG_WARNING("error parsing stia, too many bytes: %d\n", modulus);
		return;
	}
	ptr += 8;
	this->SampleSizes.resize(numEntries);
	for (int i = 0; i < numEntries; i++)
	{
		this->SampleSizes[i] = *(UINT32*)(ptr + i * 4);
	}
}

bool CMSampleBuffer::extractPPS()
{
	int magic = 0x64617476;	//datv
	for (size_t i = 0; i < this->FormatDescription.Extensions.size() - 4; i++)
	{
		if (memcmp(this->FormatDescription.Extensions.data() + i, &magic, 4) == 0)
		{
			const unsigned char* ptr = this->FormatDescription.Extensions.data() + i + 4;
			//int length_datv = *(int*)(ptr - 8);


			unsigned char ppsLength = ptr[7];
			this->FormatDescription.PPS.resize(ppsLength);
			memcpy(this->FormatDescription.PPS.data(), ptr + 8, ppsLength);

			unsigned char spsLength = ptr[10 + ppsLength];
			this->FormatDescription.SPS.resize(spsLength);
			memcpy(this->FormatDescription.SPS.data(), ptr + 11 + ppsLength, spsLength);


			this->FormatDescription.SPS_and_PPS.resize(11 + ppsLength + spsLength + 4);
			memcpy(this->FormatDescription.SPS_and_PPS.data(), ptr, 11 + ppsLength + spsLength + 4);



			return true;
		}
	}
	return false;
}

void CMSampleBuffer::parseFormatDescriptor(const unsigned char* ptr)
{
	//int length = *(int*)(ptr + 0);
	int magic = *(int*)(ptr + 4);
	if (magic != FormatDescriptorMagic)
	{
		LOG_WARNING("unknown magic type:(0x%x)\n", magic);
		return;
	}
	ptr += 8;

	//mediaType
	int length_mediaType = *(int*)(ptr + 0);
	int magic_mediaType = *(int*)(ptr + 4);
	if (magic_mediaType != MediaTypeMagic)
	{
		LOG_WARNING("unknown magic_mediaType type:(0x%x)\n", magic_mediaType);
		return;
	}
	if (length_mediaType != 12)
	{
		LOG_WARNING("invalid length_mediaType for media type: %d, should be 12\n", length_mediaType);
		return;
	}
	int mediaType = *(int*)(ptr + 8);
	ptr += 12;

	if (mediaType == MediaTypeSound)
	{
		this->FormatDescription.MediaType = MediaTypeSound;
		//音频
		int length_asbd = *(int*)(ptr + 0);
		int magic_asbd = *(int*)(ptr + 4);
		if (magic_asbd != AudioStreamBasicDescriptionMagic)
		{
			LOG_WARNING("unknown magic_asbd type:(0x%x)\n", magic_asbd);
			return;
		}
		memcpy(&this->FormatDescription.AudioStreamBasicDescription, ptr + 8, length_asbd - 8);
	}
	else if (mediaType == MediaTypeVideo)
	{
		this->FormatDescription.MediaType = MediaTypeVideo;
		//视频
		//int length_vdim = *(int*)(ptr + 0);
		int magic_vdim = *(int*)(ptr + 4);
		if (magic_vdim != VideoDimensionMagic)
		{
			LOG_WARNING("unknown magic_vdim type:(0x%x)\n", magic_vdim);
			return;
		}
		this->FormatDescription.VideoDimensionWidth = *(UINT32*)(ptr + 8);
		this->FormatDescription.VideoDimensionHeight = *(UINT32*)(ptr + 12);
		ptr += 16;

		//codc
		//int length_codc = *(int*)(ptr + 0);
		int magic_codc = *(int*)(ptr + 4);
		if (magic_codc != CodecMagic)
		{
			LOG_WARNING("unknown magic_codc type:(0x%x)\n", magic_codc);
			return;
		}
		this->FormatDescription.Codec = *(UINT32*)(ptr + 8);
		ptr += 12;

		//extn
		int length_extn = *(int*)(ptr + 0);
		int magic_extn = *(int*)(ptr + 4);
		if (magic_extn != ExtensionMagic)
		{
			LOG_WARNING("unknown magic_extn type:(0x%x)\n", magic_extn);
			return;
		}
		this->FormatDescription.Extensions.resize(length_extn - 8);
		memcpy(this->FormatDescription.Extensions.data(), ptr + 8, length_extn - 8);
		ptr += length_extn;
		//LOG_DEBUG("Extensions : %s", IosDevice::GetVectorHexString2(this->FormatDescription.Extensions).c_str());
		//TODO : pps, sps 在this->FormatDescription.Extensions字典里面
		this->extractPPS();
	}
	else
	{
		LOG_WARNING("invalid media type: 0x%x\n", mediaType);
		return;
	}


}

CMSampleBuffer::CMSampleBuffer(const std::vector<unsigned char>& vecCMSampleBuffer, UINT32 MediaType, bool* pbError)
{
	this->MediaType = MediaType;
	this->HasFormatDescription = false;
	int length = *(int*)(vecCMSampleBuffer.data() + 0);
	int magic = *(int*)(vecCMSampleBuffer.data() + 4);
	if (length > (int)vecCMSampleBuffer.size())
	{
		LOG_WARNING("invalid length in header: %d but only received: %d bytes\n", length, (int)vecCMSampleBuffer.size());
		return;
	}
	if (magic != sbuf)
	{
		LOG_WARNING("unknown magic type:(0x%x)\n", magic);
		return;
	}
	const unsigned char* ptr = vecCMSampleBuffer.data() + 8;
	const unsigned char* ptr_end = vecCMSampleBuffer.data() + vecCMSampleBuffer.size();
	int length2;
	int magic2;
	try
	{
		while (ptr < ptr_end - 8)
		{
			length2 = *(int*)(ptr + 0);
			magic2 = *(int*)(ptr + 4);
			switch (magic2)
			{
			case opts:
				memcpy(&this->OutputPresentationTimestamp, ptr + 8, 24);
				//ptr += 32;
				ptr += length2;
				break;
			case stia:
				this->parseStia(ptr);
				ptr += length2;
				break;
			case sdat:
				this->SampleData.resize(length2 - 8);
				memcpy(this->SampleData.data(), ptr + 8, length2 - 8);
				ptr += length2;
				break;
			case nsmp:
				if (length2 != 12)
				{
					LOG_WARNING("invalid length for nsmp %d, should be 12\n", length2);
					return;
				}
				this->NumSamples = *(int*)(ptr + 8);
				ptr += 12;
				break;
			case ssiz:
				this->parseSampleSizeArray(ptr);
				ptr += length2;
				break;
			case FormatDescriptorMagic:
				this->HasFormatDescription = true;
				this->parseFormatDescriptor(ptr);
				ptr += length2;
				break;
			case satt:
				this->Attachments.resize(length2 - 8);
				memcpy(this->Attachments.data(), ptr + 8, length2 - 8);
				ptr += length2;
				break;
			case sary:
				this->Sary.resize(length2 - 8);
				memcpy(this->Sary.data(), ptr + 8, length2 - 8);
				ptr += length2;
				break;
			default:
				LOG_WARNING("unknown magic2 type:(0x%x) length2:(%d)\n", magic2, length2);
				if (pbError) *pbError = true;
				//DebugBreak();
				return;
			}
		}
	}
	catch (...)
	{
		LOG_WARNING("CMSampleBuffer::CMSampleBuffer excption");
	}
	if (pbError)*pbError = false;
}
