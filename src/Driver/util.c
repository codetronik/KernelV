#include "util.h"


PUCHAR MemSearch(PUCHAR pBuffer, int nBufferSize, PUCHAR pPattern, int nPatternSize)
{
	for (int i = 0; i <= nBufferSize - nPatternSize; i++)
	{
		int j = 0;
		for (j = 0; j < nPatternSize; j++)
		{
			if (pBuffer[i + j] != pPattern[j])
			{
				break;
			}

		}
		if (j == nPatternSize)
		{
			return pBuffer + i;
		}
	}
	return NULL;
}