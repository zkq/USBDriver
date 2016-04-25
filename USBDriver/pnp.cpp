
#include "usbdriver.h"

NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS OnRequestComplete(PDEVICE_OBJECT junk, PIRP pIrp, PKEVENT pEvent);
VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list);



#pragma PAGEDCODE
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter pnp"));
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

	MyDbgPrint((" pnp request (%s)", fcnname[fcn]));
	status = (*fcntab[fcn])(pdx, pIrp);

	MyDbgPrint((" Leave pnp**************"));
	return status;
}

#pragma PAGEDCODE
NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(pdx->NextStackDevice, pIrp);
}


#pragma PAGEDCODE
NTSTATUS PnpStartDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	NTSTATUS status;
	
	
	status = ForwardAndWait(pdx, pIrp);
	if (!NT_SUCCESS(status))
	{
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}

	//使能设备接口
	status = IoSetDeviceInterfaceState(&pdx->ustrSymbolicName, TRUE);
	if (!NT_SUCCESS(status))
	{
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


	IoSetDeviceInterfaceState(&pdx->ustrSymbolicName, FALSE);
	RtlFreeUnicodeString(&pdx->ustrSymbolicName);

	if (pdx->deviceDesc)
	ExFreePool(pdx->deviceDesc);
	if (pdx->confDesc)
	ExFreePool(pdx->confDesc);
	if (pdx->pipeInfos)
	ExFreePool(pdx->pipeInfos);

	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	IoDeleteDevice(pdx->FunctionDevice);


	return status;
}




#pragma PAGEDCODE
NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter ForwardAndWait"));

	NTSTATUS status;
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, 
				(PIO_COMPLETION_ROUTINE)OnRequestComplete, 
				&event,
				TRUE, 
				TRUE, 
				TRUE);

	status = IoCallDriver(pdx->NextStackDevice, pIrp);
	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = pIrp->IoStatus.Status;
	}

	if (!NT_SUCCESS(status)) {
		MyDbgPrint(("pnp Lower drivers failed this Irp\n"));
	}

	return status;
}

#pragma LOCKEDCODE
NTSTATUS OnRequestComplete(PDEVICE_OBJECT pDo, PIRP pIrp, PKEVENT pEvent)
{
	KeSetEvent(pEvent, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}					