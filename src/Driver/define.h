#pragma once

#define kprintf(...) KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)) // DbgPrint()를 사용한 경우 로딩이 되지 않았던 문제 발생
#define POOL_TAG 'coTr'

