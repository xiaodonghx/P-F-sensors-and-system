#include "string.h"
#include "R2000DETAPI.h"
#include "math.h"




R2000DET::R2000DET()
{
}


R2000DET::~R2000DET()
{
}

bool R2000DET::InitAPI()
{
	char filename[] = "Config";

	FILE *f;
	f = fopen(filename, "r");
	if (f == NULL)
	{
//		perror("open file error:");//ֻ������ĺ���������errorȫ�ִ���ţ��ſ�ʹ�ã������error�����Ӧ�Ĵ�����Ϣ
		return false;
	}
	fseek(f, 0L, SEEK_END); /* ��λ���ļ�ĩβ */
	int64_t flen = ftell(f); /* �õ��ļ���С */
	ByteofConfigBuffer = flen;
	buff = new char[flen + 1];
	fseek(f, 0L, 0);
	fread(buff, sizeof(char), flen, f);
	fclose(f);
	GetGroupNum();
	b_Init = true;
	return true;
}

bool R2000DET::GetGroupNum()
{
	if (buff == nullptr)
		return false;
	AllGroupNum = *(int*)buff;
	return true;
}

bool R2000DET::StartGroup(int i_Group)
{
	if (buff == nullptr)//δ��ʼ��
		return false;
	ZoneBuffer= buff+GetConfigFilePos(i_Group, 1, buff) - sizeof(int);//��������
	ZoneofGroup = *(int*)ZoneBuffer;
//��Ϊÿ�ν�����һ�����飬���´�������
/*	char *tmp = nullptr;
	char *tmp1 = nullptr;//���ڸ���ZoneBuffer
	int64_t start = GetConfigFilePos(i_Group, 1, buff) - sizeof(int);//��������
	int64_t end = 0;
	if ((i_Group) <= AllGroupNum)
	{
		end = GetConfigFilePos(i_Group + 1, 1, buff) - sizeof(int);
	}
	int64_t size = end - start;//�൱��tmp�ĳ���
	tmp = (buff + GetConfigFilePos(i_Group, 1, buff) - sizeof(int));//����������buff

	tmp1 = new char[size + ByteofZoneBuffer];
	memcpy(tmp1, ZoneBuffer, ByteofZoneBuffer);
	memcpy(tmp1 + ByteofZoneBuffer, tmp, size);
	GroupNumInZoneBuffer++;
	memcpy(tmp1, &GroupNumInZoneBuffer, sizeof(int));
	if (ZoneBuffer != nullptr)
		delete[]ZoneBuffer;
	ZoneBuffer = tmp1;
	ByteofZoneBuffer += size;*/

	return true;
}

int64_t R2000DET::GetConfigFilePos(int i_Group, int i_Zone, char *buff)
{
	int64_t filelen1, tmpp = 0;

	int tmpq1 = 0, tmpz1 = 0;//tmpqһ�����ڶ�ȡ��ǰ����������

	char * tmp1;
	tmp1 = buff;//ָ���ƶ�������������������
	tmpz1 = (*(int*)tmp1);
	tmp1 += sizeof(int);
	tmpq1 = (*(int*)tmp1);
	tmp1 += sizeof(int);
	filelen1 = sizeof(int) * 2;//��λ
	bool flag = false;//������������1�ı�־��
	int i = 0, j = 0;

	for (i = 0; i < tmpz1; i++)
	{
		//�ҵ���Ҫɾ������������ţ����ж�λ
		for (j = 0; j < tmpq1; j++)
		{
			if (i == (i_Group - 1) && j == (i_Zone - 1))//i_Zone��2����ΪҪͣ�ڵ����Ŀ�ʼ
				break;
			tmpp = *(int64_t*)tmp1;
			filelen1 += tmpp * sizeof(CONFIG_XY) + sizeof(int64_t);
			tmp1 += tmpp * sizeof(CONFIG_XY) + sizeof(int64_t);
		}
		if (i == (i_Group - 1) && j == (i_Zone - 1))//��2����ΪҪͣ�ڵ����Ŀ�ʼ
		{
			//					filelen1 += sizeof(int);
			break;
		}
		tmpq1 = *(int*)tmp1;
		tmp1 += sizeof(int);
		filelen1 += sizeof(int);
	}

	return filelen1;
}
bool R2000DET::StopGroup(int i_Group)
{
//	ByteofZoneBuffer = sizeof(int);
	return false;
}
bool* R2000DET::isInZone(char* PointBuffer)
{
	if (PointBuffer == nullptr)
		return nullptr;
	char *Buffer = ZoneBuffer;
//	Buffer += sizeof(int);//��Ϊ������������
	int *qs = (int*)Buffer;
	Zone_Status = new bool[*qs];
	memset(Zone_Status, 0, sizeof(bool)*(*qs));

	uint64_t ds = 0;
	Buffer += sizeof(int);
	int i = 0,count = 0;//countΪ��������;
	uint64_t tmpi =0;
	float tmpx = -1;
	float tmpy = -1;
	R2000Head = *(tmpData*)PointBuffer;
	double angular_increment_real = 360 / (double)R2000Head.num_points_scan;
	PointBuffer += sizeof(R2000Head);
	uint32_t *distance;
	double ang;
	uint32_t times = (uint32_t)((R2000Head.packet_size - sizeof(R2000Head)) / 6);
	for (uint32_t i1 = 0; i1 <times ; i1++)
	{

		distance = (uint32_t*)(PointBuffer + i1 * 6);
		if (*distance == -1)
			break;
		ang = angular_increment_real*i1 + R2000Head.first_angle / 10000;
		a2b(ang, *distance / 62.5);
		//����ƽ�ƣ���(480,480)ΪԲ�ġ�ע��ԭʼconfig������������Ļ���꣬��Ҫת��
		x1 += 480;
		y1 += 480;

		for (i = 0; i < *qs; i++)
		{
			ds = *(uint64_t*)(Buffer); //+ds[0]*j*sizeof(CONFIG_XY));
			CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
			xy = (CONFIG_XY*)(Buffer + sizeof(int64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
			for (uint64_t j = 0; j < ds; j++)
			{
				if (fabs((float)y1 - xy[j].y) <= 0.1)//��Y��
				{
					tmpy = xy[j].y;
					tmpx = xy[j].x;
					if (isInOneZone(tmpx, tmpy, x1, y1))//�൱���ж�X��
					{
						//	flag = true;
						//	goto check;
						if (tmpi == 0 && i1 == 0)
							count++;
						else
						{
							if (tmpi >= 0 && (i1 - tmpi) == 1)//�ж��Ƿ�����
							{
								count++;
								tmpi = i1;
							}
							else
							{
								tmpi = i1;
								count = 1;
							}
						}
						if (count == 2)//����2����
						{
							Zone_Status[i] = true;
							count = 0;
						}
						break;
					}
				}


			}
			Buffer += sizeof(int64_t) + ds * sizeof(CONFIG_XY);
		}
		Buffer = ZoneBuffer+sizeof(int);
	}
	return Zone_Status;


	//	return -1;
	/*check:

	if (!flag)
	return -1;
	return i + 1;*/

}
bool R2000DET::isInOneZone(float tmpx, float tmpy, int x, int y)
{
	bool flag = false;
	uint64_t *ds = nullptr;
	ds = (uint64_t*)(ZoneBuffer);
	CONFIG_XY* xy = nullptr;//new CONFIG_XY[ds[0]+1];
	xy = (CONFIG_XY*)(ZoneBuffer + sizeof(uint64_t));// *(j + 1) + sizeof(CONFIG_XY)*ds[0] * j);
	for (uint64_t j = 0; j < ds[0]; j++)
	{
		if ((tmpy >= 0) && (fabs((float)tmpy - xy[j].y) <= 0.1))
		{

			if (x <= (tmpx >= xy[j].x ? tmpx : xy[j].x) && (x >= (tmpx <= xy[j].x ? tmpx : xy[j].x)))
			{
				flag = true;
				break;
			}

		}

	}
	return flag;
}

bool* R2000DET::GetStatus(char* PointBuffer)
{
	if (Zone_Status != nullptr)
	{
		delete[]Zone_Status;
		Zone_Status = nullptr;
	}
	bool* Status = isInZone(PointBuffer);


	return Status;
}
void R2000DET::a2b(double angle, double distance)
{
//	x1 = distance*cos(angle / 180 * 3.1415926535);
//	y1 = distance*sin(angle / 180 * 3.1415926535);
	y1 = distance*cos(angle / 180 * 3.1415926535);
	x1 = distance*sin(angle / 180 * 3.1415926535);
}
bool R2000DET::EndAPI()
{
	b_Init = false;
	if (Zone_Status != nullptr)
		delete[]Zone_Status;
	if (ZoneBuffer != nullptr)
		delete[]ZoneBuffer;
	if (buff != nullptr)
		delete[]buff;
	return true;
}