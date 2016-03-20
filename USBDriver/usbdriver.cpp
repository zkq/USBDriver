

#include "usbdriver.h"

#pragma INITCODE
extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	KdPrint(("Enter DriverEntry\n"));

	pDriverObject->DriverExtension->AddDevice = addDevice;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = pnp;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
		pDriverObject->MajorFunction[IRP_MJ_CREATE] =
		pDriverObject->MajorFunction[IRP_MJ_READ] =
		pDriverObject->MajorFunction[IRP_MJ_WRITE] = dispatchRoutine;
	pDriverObject->DriverUnload = unload;

	KdPrint(("Leave DriverEntry\n"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS addDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhyDeviceObject)
{
	PAGED_CODE();
	KdPrint(("Enter addDevice"));

	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device\\USBDevice");

	PDEVICE_OBJECT fdo;
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

	KdPrint(("Leave addDevice"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("Enter defaultpnphandler"));

	IoSkipCurrentIrpStackLocation(pIrp);

	KdPrint(("Leave defaultpnphandler"));
	return IoCallDriver(pdx->NextStackDevice, pIrp);

}


#pragma PAGEDCODE
NTSTATUS RemoveDeviceHandler(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("Enter removeDeviceHandler"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, pIrp);
	IoDeleteSymbolicLink(&pdx->ustrSymbolicName);

	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	IoDeleteDevice(pdx->fdo);

	KdPrint(("Leave removeDeviceHandler"));
	return status;

}

#pragma PAGEDCODE
NTSTATUS pnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("Enter pnp"));

	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pFdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	static NTSTATUS(*fcntab[])(PDEVICE_EXTENSION pdx, PIRP pIrp) =
	{
		DefaultPnpHandler,
		DefaultPnpHandler,
		RemoveDeviceHandler,
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

	KdPrint(("pnp request (%s)", fcnname[fcn]));
	status = (*fcntab[fcn])(pdx, pIrp);

	KdPrint(("Leave pnp"));
	return status;
}




#pragma PAGEDCODE
NTSTATUS dispatchRoutine(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("Enter dispatchRoutine"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave dispatchRoutine"));
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
void unload(IN PDRIVER_OBJECT pDriverObject)
{
	PAGED_CODE();
	KdPrint(("Enter unLoad"));
	KdPrint(("Leave unLoad"));
}