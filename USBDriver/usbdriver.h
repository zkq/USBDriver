
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>

#ifdef __cplusplus
}
#endif // __cplusplus


#include "cyioctl.h"


#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")


#define arraysize(p) (sizeof(p)/sizeof(p[0]))

typedef struct _PIPE_INFO
{
	UCHAR address;
	USBD_PIPE_HANDLE handle;
} PIPE_INFO, *PPIPE_INFO;

typedef struct _DEVICE_EXTERNSION
{
	PDEVICE_OBJECT fdo;
	PLIST_ENTRY pIrpListHead;
	PDEVICE_OBJECT NextStackDevice;
	UNICODE_STRING ustrDeviceName;
	UNICODE_STRING ustrSymbolicName;

	PVOID BusContext;
	USBD_HANDLE UsbdHandle;
	PUSB_DEVICE_DESCRIPTOR deviceDesc;
	PUSB_CONFIGURATION_DESCRIPTOR confDesc;
	USBD_CONFIGURATION_HANDLE confHandle;
	UCHAR pipeCount;
	PPIPE_INFO pipeInfos;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _IRP_Entry
{
	PIRP pIrp;
	LIST_ENTRY listEntry;

} IRP_Entry, *PIRP_Entry;


extern GUID guid;

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);

//StartIO 用于串行处理IO请求
VOID StartIO(IN PDEVICE_OBJECT pDev, IN PIRP pIrp);
//******************取消的IRP*********************************
//通过IoSetCancelRoutine将此例程和一个Irp关联。
//调用IoCabcelIrp时，会调用Irp的CancelIrp例程。
//当文件句柄关闭时，会枚举所有挂起的Irp，并调用其CancelIrp例程。
VOID OnCancelIrp(IN PDEVICE_OBJECT pDev, IN PIRP pIrp);

NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhyDeviceObject);


NTSTATUS Pnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS PnpStartDevice(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS PnpRemoveDevice(PDEVICE_EXTENSION pdx, PIRP pIrp);

NTSTATUS CreateFile(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS CleanUp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DispatchRoutine(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DeviceIOControl(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);


NTSTATUS UrbCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
NTSTATUS SubmitUrbSync(PDEVICE_EXTENSION DeviceExtension, PURB Urb, BOOLEAN IsUrbBuildByNewMethod = true, PIO_COMPLETION_ROUTINE SyncCompletionRoutine = UrbCompletionRoutine);
NTSTATUS SubmitUrbASync(PDEVICE_EXTENSION DeviceExtension, PIRP Irp, PURB Urb, PIO_COMPLETION_ROUTINE CompletionRoutine, PVOID CompletionContext);

//IOCTRL helper function
NTSTATUS GetStatus(PDEVICE_EXTENSION pdx, PUSHORT status, UCHAR target, UCHAR index);
NTSTATUS Feature(PDEVICE_EXTENSION pdx, BOOLEAN bset, UCHAR target, USHORT selector, UCHAR index);
NTSTATUS SetAddress(PDEVICE_EXTENSION pdx, UCHAR address);
NTSTATUS GetDeviceDesc(PDEVICE_EXTENSION pdx, bool refresh = false);
NTSTATUS GetConfDesc(PDEVICE_EXTENSION pdx, bool refresh = false);
NTSTATUS SelectConfiguration(PDEVICE_EXTENSION pdx);
NTSTATUS SelectInterface(PDEVICE_EXTENSION pdx, UCHAR altIntNum);
NTSTATUS GetConfiguration(PDEVICE_EXTENSION pdx, PUCHAR confNum);
NTSTATUS GetInterface(PDEVICE_EXTENSION pdx, PUCHAR intfNum);
NTSTATUS SendNonEP0CtlData(PDEVICE_EXTENSION pdx, UCHAR endAddress, PVOID isoInfoBuf, const ULONG isoLen, PVOID buffer, const ULONG bufLen);
NTSTATUS VendorRequest(PDEVICE_EXTENSION pdx, PSINGLE_TRANSFER single);
NTSTATUS SetPwr(PDEVICE_EXTENSION pdx, POWER_STATE state);



//IO CTL Methods
NTSTATUS GetDriverVersion(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetUsbDIVersion(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetAltIntSetting(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS SelIntface(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetAddress(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetNumEndpoints(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetPwrState(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS SetPwrState(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS SendEP0Ctl(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS SendNonEP0Ctl(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS CyclePort(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS ResetPipe(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS ResetParentPort(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetTransSize(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS SetTransSize(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetDiName(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetFriendlyName(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS AbortPipe(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS SendNonEP0Direct(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetSpeed(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS GetCurrentFrame(PDEVICE_EXTENSION pdx, PIRP pIrp);



void Unload(IN PDRIVER_OBJECT pDriverObject);


extern "C"
NTKERNELAPI UCHAR *PsGetProcessImageFileName(__in PEPROCESS Process);

void DisplayProcessName();


#ifdef DBG
#define MyDbgPrint(_x_)   DbgPrint("mydriver:");    DbgPrint _x_;  DbgPrint("\n");
//#define MyDbgPrint(_x_)   DbgPrint _x_;  
#else
#define MyDbgPrint(_x_)
#endif