
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



extern GUID guid;
extern UNICODE_STRING registryPath;


typedef struct _PIPE_INFO
{
	UCHAR address;
	BOOLEAN open;
	USBD_PIPE_HANDLE handle;
} PIPE_INFO, *PPIPE_INFO;
typedef struct _DEVICE_EXTERNSION
{
	PDEVICE_OBJECT FunctionDevice;
	PDEVICE_OBJECT NextStackDevice;
	PDEVICE_OBJECT PhysicalDevice;

	UNICODE_STRING ustrDeviceName;
	UNICODE_STRING ustrSymbolicName;
	
	DEVICE_POWER_STATE pwrState;

	FAST_MUTEX myMutex;
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




extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);

//******************取消的IRP*********************************
//通过IoSetCancelRoutine将此例程和一个Irp关联。
//调用IoCabcelIrp时，会调用Irp的CancelIrp例程。
//当文件句柄关闭时，会枚举所有挂起的Irp，并调用其CancelIrp例程。
VOID OnCancelIrp(IN PDEVICE_OBJECT pDev, IN PIRP pIrp);

NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhyDeviceObject);


NTSTATUS DispatchPnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DefaultPnpHandler(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp);
NTSTATUS PnpStartDevice(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp);
NTSTATUS PnpRemoveDevice(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp);

NTSTATUS DispatchCreate(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DispatchCleanUp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DispatchRoutine(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS DispatchDeviceIOControl(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);


NTSTATUS DispatchPower(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp);
NTSTATUS SystemQueryPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP Irp);
NTSTATUS SystemSetPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP Irp);
NTSTATUS DeviceQueryPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP Irp);
NTSTATUS DeviceSetPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP Irp);
VOID QueryPwrComplete(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus);
NTSTATUS OnQueryRequestComplete(PDEVICE_OBJECT pdo, PIRP pIrp, PVOID context);
VOID SetPwrComplete(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus);
NTSTATUS OnSetRequestComplete(PDEVICE_OBJECT pdo, PIRP pIrp, PVOID context);









NTSTATUS UrbAsyncSyncCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
NTSTATUS UrbSyncCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
NTSTATUS SubmitUrbSync(PDEVICE_EXTENSION DeviceExtension, PURB Urb, BOOLEAN IsUrbBuildByNewMethod = TRUE, PIO_COMPLETION_ROUTINE SyncCompletionRoutine = UrbSyncCompletionRoutine);
NTSTATUS SubmitUrbAsync(PDEVICE_EXTENSION DeviceExtension, PIRP Irp, PURB Urb, BOOLEAN IsUrbBuildByNewMethod = TRUE, PIO_COMPLETION_ROUTINE CompletionRoutine = UrbAsyncSyncCompletionRoutine);

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
NTSTATUS SendNonEP0CtlData(PDEVICE_EXTENSION pdx, PIRP pIrp, UCHAR endAddress, PVOID isoInfoBuf, const ULONG isoLen, PVOID buffer, const ULONG bufLen);
NTSTATUS VendorRequest(PDEVICE_EXTENSION pdx, PSINGLE_TRANSFER single);
NTSTATUS SetPwr(PDEVICE_EXTENSION pdx, POWER_STATE state);
NTSTATUS GetRegister(IN PCWSTR itemName, IN PCWSTR keyName, OUT PSTR value, IN ULONG len, OUT PULONG realSize);
NTSTATUS AbortPipe(PDEVICE_EXTENSION pdx, UCHAR address);
NTSTATUS ResetPipe(PDEVICE_EXTENSION pdx, UCHAR address);
NTSTATUS ResetParentPort(PDEVICE_EXTENSION pdx);

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