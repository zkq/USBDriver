
#include "usbdriver.h"
#include <windef.h>


#pragma INITCODE
extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	KdPrint(("mydriver:Enter DriverEntry\n"));
	
	DisplayProcessName();

	pDriverObject->DriverExtension->AddDevice = addDevice;

	//即插即用消息
	pDriverObject->MajorFunction[IRP_MJ_PNP] = pnp;
	//电源管理消息
	pDriverObject->MajorFunction[IRP_MJ_POWER] = dispatchRoutine;

	//createfile
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = dispatchRoutine; 
	//closehandle
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = dispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = dispatchRoutine;
	//deviceiocontrol
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = deviceIOControl;
	//readfile
	pDriverObject->MajorFunction[IRP_MJ_READ] = dispatchRoutine;
	//writefile
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = dispatchRoutine;


	pDriverObject->DriverUnload = unload;

	pDriverObject->DeviceObject;
	pDriverObject->DriverExtension->AddDevice;
	pDriverObject->DriverExtension->Count;
	pDriverObject->DriverExtension->DriverObject;
	pDriverObject->DriverExtension->ServiceKeyName;
	pDriverObject->DriverInit;
	pDriverObject->DriverName;
	pDriverObject->DriverSection;
	pDriverObject->DriverSize;
	pDriverObject->DriverStart;
	pDriverObject->DriverStartIo;
	pDriverObject->DriverUnload;
	pDriverObject->FastIoDispatch;
	pDriverObject->Flags;
	pDriverObject->HardwareDatabase;
	pDriverObject->MajorFunction;
	pDriverObject->Size;
	pDriverObject->Type;


	KdPrint(("mydriver:Leave DriverEntry**************\n"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS addDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhyDeviceObject)
{
	PAGED_CODE();
	KdPrint(("mydriver:Enter addDevice"));
	DisplayProcessName();

	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device\\USBDevice");

	PDEVICE_OBJECT fdo;
	fdo->ActiveThreadCount;
	fdo->AlignmentRequirement;
	fdo->AttachedDevice;
	fdo->Characteristics;
	fdo->CurrentIrp;
	fdo->DeviceExtension;
	fdo->DeviceLock;
	fdo->DeviceObjectExtension;
	fdo->DeviceQueue;
	fdo->DeviceType;
	fdo->Dpc;
	fdo->DriverObject;
	fdo->Flags;
	fdo->NextDevice;
	fdo->Queue;
	fdo->ReferenceCount;
	fdo->Reserved;
	fdo->SectorSize;
	fdo->SecurityDescriptor;
	fdo->Size;
	fdo->Spare1;
	fdo->StackSize;
	fdo->Timer;
	fdo->Type;
	fdo->Vpb;



	NTSTATUS status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), &devName, FILE_DEVICE_SERIAL_PORT, 0, false, &fdo);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	PDEVICE_EXTENSION pDevEx = reinterpret_cast<PDEVICE_EXTENSION>(fdo->DeviceExtension);
	pDevEx->fdo = fdo;
	pDevEx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, pPhyDeviceObject);


	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\USBDevice");

	pDevEx->ustrDeviceName = devName;
	pDevEx->ustrSymbolicName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteSymbolicLink(&pDevEx->ustrSymbolicName);
		status = IoCreateSymbolicLink(&symLinkName, &devName);
		if (!NT_SUCCESS(status))
		{
			return status;
		}
	}

	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	KdPrint(("mydriver:Leave addDevice**************"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS pnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("mydriver:Enter pnp"));
	DisplayProcessName();

	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pFdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	static NTSTATUS(*fcntab[])(PDEVICE_EXTENSION pdx, PIRP pIrp) =
	{
		PnpStartDevice,
		DefaultPnpHandler,
		PnpRemoveDevice,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler, 
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
	};

	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
	{
		status = DefaultPnpHandler(pdx, pIrp);

		return status;
	}

	static char *fcnname[] =
	{
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCLE_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_PNP_DEVICE_STATE",
		"IRP_MN_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",
	};

	KdPrint(("mydriver:pnp request (%s)", fcnname[fcn]));
	status = (*fcntab[fcn])(pdx, pIrp);

	KdPrint(("mydriver:Leave pnp**************"));
	return status;
}

#pragma PAGEDCODE
NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(pdx->NextStackDevice, pIrp);
}


#pragma LOCKEDCODE
NTSTATUS OnRequestComplete(PDEVICE_OBJECT junk, PIRP pIrp, PKEVENT pEvent)
{
	KeSetEvent(pEvent, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}



#pragma PAGEDCODE
NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("mydriver:Enter ForwardAndWait"));

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, (PIO_COMPLETION_ROUTINE)OnRequestComplete, &event, TRUE, TRUE, TRUE);

	IoCallDriver(pdx->NextStackDevice, pIrp);
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

	KdPrint(("mydriver:leave ForwardAndWait"));
	return pIrp->IoStatus.Status;
}

#pragma PAGEDCODE
NTSTATUS PnpStartDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	NTSTATUS status = ForwardAndWait(pdx, pIrp);

	if (!NT_SUCCESS(status))
	{
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}



#pragma PAGEDCODE
NTSTATUS PnpRemoveDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, pIrp);
	IoDeleteSymbolicLink(&pdx->ustrSymbolicName);

	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	IoDeleteDevice(pdx->fdo);
	return status;
}

#pragma PAGEDCODE
NTSTATUS deviceIOControl(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("mydriver:Enter deviceIOControl"));
	DisplayProcessName();

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

/*	switch (code)
	{
	case 1:
		UCHAR *outBuffer = (UCHAR*)pIrp->AssociatedIrp.SystemBuffer;
		memset(outBuffer, 0xff, outLen);
		
		break;
	default:
		break;
	}*/

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = outLen;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("mydriver:Leave deviceIOControl**************"));
	return STATUS_SUCCESS;

}


#pragma PAGEDCODE
NTSTATUS dispatchRoutine(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("mydriver:Enter dispatchRoutine"));
	DisplayProcessName();

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("mydriver:Leave dispatchRoutine**************"));
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
void unload(IN PDRIVER_OBJECT pDriverObject)
{
	PAGED_CODE();
	KdPrint(("mydriver:Enter unLoad"));
	DisplayProcessName();
	KdPrint(("mydriver:Leave unLoad**************"));
}