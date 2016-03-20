
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <wdm.h>

#ifdef __cplusplus
}
#endif // __cplusplus


#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof(p[0]))

typedef struct _DEVICE_EXTERNSION
{
	PDEVICE_OBJECT fdo;
	PDEVICE_OBJECT NextStackDevice;
	UNICODE_STRING ustrDeviceName;
	UNICODE_STRING ustrSymbolicName;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


NTSTATUS addDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhyDeviceObject);
NTSTATUS pnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS dispatchRoutine(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
void unload(IN PDRIVER_OBJECT pDriverObject);

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);

