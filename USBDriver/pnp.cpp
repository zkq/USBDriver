
#include "usbdriver.h"

NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP pIrp);
NTSTATUS OnRequestComplete(PDEVICE_OBJECT junk, PIRP pIrp, PKEVENT pEvent);
VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list);



#pragma PAGEDCODE
NTSTATUS Pnp(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
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
	NTSTATUS status = ForwardAndWait(pdx, pIrp);

	if (!NT_SUCCESS(status))
	{
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}
	MyDbgPrint((" show raw resources"));
	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(pIrp);
	PCM_PARTIAL_RESOURCE_LIST raw;
	if (stack->Parameters.StartDevice.AllocatedResources)
	{
		raw = &stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
		MyDbgPrint((" show raw resources"));
		ShowResources(raw);
	}

	else
		raw = NULL;



	PCM_PARTIAL_RESOURCE_LIST translated;
	if (stack->Parameters.StartDevice.AllocatedResourcesTranslated)
	{
		translated = &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;
		MyDbgPrint((" show translated resources"));
		ShowResources(translated);
	}

	else
		translated = NULL;


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

	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	IoDeleteDevice(pdx->fdo);
	return status;
}




#pragma PAGEDCODE
NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter ForwardAndWait"));

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, (PIO_COMPLETION_ROUTINE)OnRequestComplete, &event, TRUE, TRUE, TRUE);

	IoCallDriver(pdx->NextStackDevice, pIrp);
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

	MyDbgPrint((" leave ForwardAndWait"));
	return pIrp->IoStatus.Status;
}

#pragma LOCKEDCODE
NTSTATUS OnRequestComplete(PDEVICE_OBJECT junk, PIRP pIrp, PKEVENT pEvent)
{
	KeSetEvent(pEvent, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

#pragma PAGEDCODE
VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list)
{
	//Ã¶¾Ù×ÊÔ´
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = list->PartialDescriptors;
	ULONG nres = list->Count;
	ULONG i;

	for (i = 0; i < nres; ++i, ++resource)
	{						// for each resource
		ULONG type = resource->Type;

		static char* name[] = {
			"CmResourceTypeNull",
			"CmResourceTypePort",
			"CmResourceTypeInterrupt",
			"CmResourceTypeMemory",
			"CmResourceTypeDma",
			"CmResourceTypeDeviceSpecific",
			"CmResourceTypeBusNumber",
			"CmResourceTypeDevicePrivate",
			"CmResourceTypeAssignedResource",
			"CmResourceTypeSubAllocateFrom",
		};

		MyDbgPrint((" type %s", type < arraysize(name) ? name[type] : "unknown"));

		switch (type)
		{					// select on resource type
		case CmResourceTypePort:
		case CmResourceTypeMemory:
			MyDbgPrint((" start %8X%8.8lX length %X\n",
				resource->u.Port.Start.HighPart, resource->u.Port.Start.LowPart,
				resource->u.Port.Length));
			break;

		case CmResourceTypeInterrupt:
			MyDbgPrint(("  level %X, vector %X, affinity %X\n",
				resource->u.Interrupt.Level, resource->u.Interrupt.Vector,
				resource->u.Interrupt.Affinity));
			break;

		case CmResourceTypeDma:
			MyDbgPrint(("  channel %d, port %X\n",
				resource->u.Dma.Channel, resource->u.Dma.Port));
		}
	}
}							