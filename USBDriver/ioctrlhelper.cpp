

#include "usbdriver.h"

NTSTATUS GetStatus(PDEVICE_EXTENSION pdx, PUSHORT statusInfo, UCHAR target, UCHAR index)
{
	PURB urb = NULL;
	NTSTATUS status;
	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(_URB_CONTROL_GET_STATUS_REQUEST));
	if (!urb)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		MyDbgPrint(("allocate urb failed"));
		goto Exit;
	}

	USHORT function = 0;
	switch (target)
	{
	case BMREQUEST_TO_DEVICE:
		function = URB_FUNCTION_GET_STATUS_FROM_DEVICE;
		break;
	case BMREQUEST_TO_INTERFACE:
		function = URB_FUNCTION_GET_STATUS_FROM_INTERFACE;
		break;
	case BMREQUEST_TO_ENDPOINT:
		function = URB_FUNCTION_GET_STATUS_FROM_ENDPOINT;
		break;
	case BMREQUEST_TO_OTHER:
		function = URB_FUNCTION_GET_STATUS_FROM_OTHER;
		break;
	}
	if (function == 0)
	{
		MyDbgPrint(("invalid function"));
		goto Exit;
	}


	urb->UrbControlGetStatusRequest.Hdr.Function = function;
	urb->UrbControlGetStatusRequest.Hdr.Length = sizeof(_URB_CONTROL_GET_STATUS_REQUEST);
	urb->UrbControlGetStatusRequest.TransferBufferLength = 2;
	urb->UrbControlGetStatusRequest.TransferBuffer = statusInfo;
	urb->UrbControlGetStatusRequest.Index = index;
	status = SubmitUrbSync(pdx, urb, FALSE);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("feature failed"));
	}

Exit:
	if (urb)
	{
		ExFreePool(urb);
		urb = NULL;
	}
	return status;

}


NTSTATUS Feature(PDEVICE_EXTENSION pdx, BOOLEAN bset, UCHAR target, USHORT selector, UCHAR index)
{
	PURB urb = NULL;
	NTSTATUS status;
	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(_URB_CONTROL_FEATURE_REQUEST));
	if (!urb)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		MyDbgPrint(("allocate urb failed"));
		goto Exit;
	}

	USHORT function = 0;
	if (bset)
	{
		switch (target)
		{
			case BMREQUEST_TO_DEVICE:
				function = URB_FUNCTION_SET_FEATURE_TO_DEVICE;
			break;
			case BMREQUEST_TO_INTERFACE:
				function = URB_FUNCTION_SET_FEATURE_TO_INTERFACE;
				break;
			case BMREQUEST_TO_ENDPOINT:
				function = URB_FUNCTION_SET_FEATURE_TO_ENDPOINT;
				break;
			case BMREQUEST_TO_OTHER:
				function = URB_FUNCTION_SET_FEATURE_TO_OTHER;
				break;
		}
	}
	else{
		switch (target)
		{
			case BMREQUEST_TO_DEVICE:
				function = URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE;
			break;
			case BMREQUEST_TO_INTERFACE:
				function = URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE;
				break;
			case BMREQUEST_TO_ENDPOINT:
				function = URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT;
				break;
			case BMREQUEST_TO_OTHER:
				function = URB_FUNCTION_CLEAR_FEATURE_TO_OTHER;
				break;
		}
	}
	if (function == 0)
	{
		MyDbgPrint(("invalid function"));
		goto Exit;
	}


	urb->UrbControlFeatureRequest.Hdr.Function = function;
	urb->UrbControlFeatureRequest.Hdr.Length = sizeof(_URB_CONTROL_FEATURE_REQUEST);
	urb->UrbControlFeatureRequest.FeatureSelector = selector;
	urb->UrbControlFeatureRequest.Index = index;
	status = SubmitUrbSync(pdx, urb, FALSE);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("feature failed"));
	}

Exit:
	if (urb)
	{
		ExFreePool(urb);
		urb = NULL;
	}
	return status;

}


NTSTATUS SetAddress(PDEVICE_EXTENSION pdx, UCHAR address)
{
	PURB urb = NULL;
	NTSTATUS status;
	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(_URB_CONTROL_TRANSFER));
	if (!urb)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		MyDbgPrint(("allocate urb failed"));
		goto Exit;
	}

	urb->UrbControlTransfer.Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER;
	urb->UrbControlTransfer.Hdr.Length = sizeof(_URB_CONTROL_TRANSFER);
	urb->UrbControlTransfer.PipeHandle = NULL;
	urb->UrbControlTransfer.TransferFlags = USBD_DEFAULT_PIPE_TRANSFER;
	urb->UrbControlTransfer.TransferBufferLength = 0;
	urb->UrbControlTransfer.TransferBuffer = NULL;
	urb->UrbControlTransfer.TransferBufferMDL = NULL;
	//bmRequestType
	urb->UrbControlTransfer.SetupPacket[0] = 0;
	//bRequest
	urb->UrbControlTransfer.SetupPacket[1] = USB_REQUEST_SET_ADDRESS;
	//wValue
	urb->UrbControlTransfer.SetupPacket[2] = address;
	urb->UrbControlTransfer.SetupPacket[3] = 0;
	//wIndex
	urb->UrbControlTransfer.SetupPacket[4] = 0;
	urb->UrbControlTransfer.SetupPacket[5] = 0;
	//wLength
	urb->UrbControlTransfer.SetupPacket[6] = 0;
	urb->UrbControlTransfer.SetupPacket[7] = 0;

	status = SubmitUrbSync(pdx, urb, FALSE);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("get intfNum failed"));
	}

Exit:
	if (urb)
	{
		ExFreePool(urb);
		urb = NULL;
	}
	return status;
}


NTSTATUS GetDeviceDesc(PDEVICE_EXTENSION pdx, bool refresh)
{
	NTSTATUS status;
	//已经获取而且不用刷新
	if (pdx->deviceDesc != NULL && !refresh)
	{
		MyDbgPrint(("device desc old"));
		status = STATUS_SUCCESS;
		return status;
	}

	ExAcquireFastMutex(&pdx->myMutex);
	if (pdx->deviceDesc)
	{
		ExFreePool(pdx->deviceDesc);
	}
	pdx->deviceDesc = (PUSB_DEVICE_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeof(USB_DEVICE_DESCRIPTOR));
	ExReleaseFastMutex(&pdx->myMutex);

	if (!pdx->deviceDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	UsbBuildGetDescriptorRequest(urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, pdx->deviceDesc, NULL, sizeof(USB_DEVICE_DESCRIPTOR), NULL);
	//ExAcquireFastMutex(&pdx->myMutex);
	status = SubmitUrbSync(pdx, urb);
	//ExReleaseFastMutex(&pdx->myMutex);
	USBD_UrbFree(pdx->UsbdHandle, urb);
	return status;
}


NTSTATUS GetConfDesc(PDEVICE_EXTENSION pdx, bool refresh)
{
	NTSTATUS status;
	//已经获取而且不用刷新
	if (pdx->confDesc != NULL && !refresh)
	{
		MyDbgPrint(("device desc old"));
		status = STATUS_SUCCESS;
		return status;
	}

	ExAcquireFastMutex(&pdx->myMutex);
	if (pdx->confDesc)
	{
		ExFreePool(pdx->confDesc);
	}
	//第一步获取大小
	pdx->confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeof(USB_CONFIGURATION_DESCRIPTOR));
	ExReleaseFastMutex(&pdx->myMutex);


	if (!pdx->confDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	UsbBuildGetDescriptorRequest(urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pdx->confDesc, NULL, sizeof(USB_CONFIGURATION_DESCRIPTOR), NULL);
	//ExAcquireFastMutex(&pdx->myMutex);
	status = SubmitUrbSync(pdx, urb);
	//ExReleaseFastMutex(&pdx->myMutex);

	USBD_UrbFree(pdx->UsbdHandle, urb);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	//第二步获取完整的
	ULONG size = pdx->confDesc->wTotalLength;
	ExAcquireFastMutex(&pdx->myMutex);
	ExFreePool(pdx->confDesc);
	pdx->confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, size);
	ExReleaseFastMutex(&pdx->myMutex);
	if (!pdx->confDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	PURB urb2;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb2);
	UsbBuildGetDescriptorRequest(urb2, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pdx->confDesc, NULL, size, NULL);

	//ExAcquireFastMutex(&pdx->myMutex);
	status = SubmitUrbSync(pdx, urb2);
	//ExReleaseFastMutex(&pdx->myMutex);

	USBD_UrbFree(pdx->UsbdHandle, urb2);
	return status;
}


NTSTATUS GetConfiguration(PDEVICE_EXTENSION pdx, PUCHAR confNum)
{
	PURB urb = NULL;
	NTSTATUS status;
	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(_URB_CONTROL_GET_CONFIGURATION_REQUEST));
	if (!urb)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		MyDbgPrint(("allocate urb failed"));
		goto Exit;
	}
	
	urb->UrbControlGetConfigurationRequest.Hdr.Function = URB_FUNCTION_GET_CONFIGURATION;
	urb->UrbControlGetConfigurationRequest.Hdr.Length = sizeof(_URB_CONTROL_GET_CONFIGURATION_REQUEST);
	urb->UrbControlGetConfigurationRequest.TransferBufferLength = 1;
	urb->UrbControlGetConfigurationRequest.TransferBuffer = confNum;
	urb->UrbControlGetConfigurationRequest.TransferBufferMDL = NULL; 

	status = SubmitUrbSync(pdx, urb, FALSE);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("get confNum failed"));
	}

Exit:
	if (urb)
	{
		ExFreePool(urb);
		urb = NULL;
	}
	return status;
}


NTSTATUS SelectConfiguration(PDEVICE_EXTENSION pdx)
{
	PURB urb = NULL;

	NTSTATUS  status;

	PUSBD_INTERFACE_LIST_ENTRY   interfaceList = NULL;
	PUSB_INTERFACE_DESCRIPTOR    interfaceDescriptor = NULL;
	PUSBD_INTERFACE_INFORMATION  Interface = NULL;

	ULONG                        interfaceIndex;

	if (!pdx->confDesc)
	{
		status = GetConfDesc(pdx);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
	}



	// Allocate an array for the list of interfaces
	// The number of elements must be one more than number of interfaces.
	interfaceList = (PUSBD_INTERFACE_LIST_ENTRY)ExAllocatePool(
		NonPagedPool,
		sizeof(USBD_INTERFACE_LIST_ENTRY) *
		(pdx->confDesc->bNumInterfaces + 1));

	if (!interfaceList)
	{
		//Failed to allocate memory
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	// Initialize the array by setting all members to NULL.
	RtlZeroMemory(interfaceList, sizeof(
		USBD_INTERFACE_LIST_ENTRY) *
		(pdx->confDesc->bNumInterfaces + 1));
	// Enumerate interfaces in the configuration.
	PUCHAR StartPosition = (PUCHAR)pdx->confDesc;
	for (interfaceIndex = 0;
		interfaceIndex < pdx->confDesc->bNumInterfaces;
		interfaceIndex++)
	{
		if (interfaceIndex == 0)
			interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)(StartPosition + pdx->confDesc->bLength);
		else
			interfaceDescriptor = NULL;

		if (!interfaceDescriptor)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Exit;
		}

		// Set the interface entry
		interfaceList[interfaceIndex].InterfaceDescriptor = interfaceDescriptor;
		interfaceList[interfaceIndex].Interface = NULL;

		// Move the position to the next interface descriptor
		StartPosition = (PUCHAR)interfaceDescriptor + interfaceDescriptor->bLength;

	}

	// Make sure that the InterfaceDescriptor member of the last element to NULL.
	interfaceList[pdx->confDesc->bNumInterfaces].InterfaceDescriptor = NULL;

	// Allocate and build an URB for the select-configuration request.
	status = USBD_SelectConfigUrbAllocateAndBuild(
		pdx->UsbdHandle,
		pdx->confDesc,
		interfaceList,
		&urb);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = SubmitUrbSync(
		pdx,
		urb);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}
	pdx->confHandle = urb->UrbSelectConfiguration.ConfigurationHandle;

	// Enumerate the pipes in the interface information array, which is now filled with pipe
	// information.

	for (interfaceIndex = 0;
		interfaceIndex < pdx->confDesc->bNumInterfaces;
		interfaceIndex++)
	{
		ULONG i;

		Interface = interfaceList[interfaceIndex].Interface;

		KIRQL oldIrql;
		ExAcquireFastMutex(&pdx->myMutex);
		pdx->pipeCount = Interface->NumberOfPipes;
		if (pdx->pipeInfos)
			ExFreePool(pdx->pipeInfos);
		pdx->pipeInfos = (PPIPE_INFO)ExAllocatePool(NonPagedPool, sizeof(PIPE_INFO) * pdx->pipeCount);
		ExReleaseFastMutex(&pdx->myMutex);

		if (!pdx->pipeInfos)
		{
			MyDbgPrint(("allocate pipeinfo failed"));
			goto Exit;
		}

		ExAcquireFastMutex(&pdx->myMutex);
		for (i = 0; i < Interface->NumberOfPipes; i++)
		{
			pdx->pipeInfos[i].address = Interface->Pipes[i].EndpointAddress;
			pdx->pipeInfos[i].handle = Interface->Pipes[i].PipeHandle;
			pdx->pipeInfos[i].open = TRUE;
		}
		ExReleaseFastMutex(&pdx->myMutex);
	}

Exit:

	if (interfaceList)
	{
		ExFreePool(interfaceList);
		interfaceList = NULL;
	}

	if (urb)
	{
		USBD_UrbFree(pdx->UsbdHandle, urb);
	}


	return status;

}


NTSTATUS GetInterface(PDEVICE_EXTENSION pdx, PUCHAR intfNum)
{
	PURB urb = NULL;
	NTSTATUS status;
	urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(_URB_CONTROL_GET_INTERFACE_REQUEST));
	if (!urb)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		MyDbgPrint(("allocate urb failed"));
		goto Exit;
	}

	urb->UrbControlGetInterfaceRequest.Hdr.Function = URB_FUNCTION_GET_INTERFACE;
	urb->UrbControlGetInterfaceRequest.Hdr.Length = sizeof(_URB_CONTROL_GET_INTERFACE_REQUEST);
	urb->UrbControlGetInterfaceRequest.TransferBufferLength = 1;
	urb->UrbControlGetInterfaceRequest.TransferBuffer = intfNum;
	urb->UrbControlGetInterfaceRequest.TransferBufferMDL = NULL;
	urb->UrbControlGetInterfaceRequest.Interface = 0;
	status = SubmitUrbSync(pdx, urb, FALSE);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("get intfNum failed"));
	}

Exit:
	if (urb)
	{
		ExFreePool(urb);
		urb = NULL;
	}
	return status;
}


NTSTATUS SelectInterface(PDEVICE_EXTENSION pdx, UCHAR altIntNum)
{
	PURB urb = NULL;

	NTSTATUS  status;

	PUSBD_INTERFACE_LIST_ENTRY   interfaceList = NULL;
	PUSB_INTERFACE_DESCRIPTOR    interfaceDescriptor = NULL;
	PUSBD_INTERFACE_INFORMATION  Interface = NULL;

	if (!pdx->confDesc)
	{
		status = GetConfDesc(pdx);
		if (!NT_SUCCESS(status))
		{
			goto Exit;
		}
	}


	// Allocate an array for the list of interfaces
	// The number of elements must be one more than number of interfaces.
	interfaceList = (PUSBD_INTERFACE_LIST_ENTRY)ExAllocatePool(
		NonPagedPool,
		sizeof(USBD_INTERFACE_LIST_ENTRY));

	if (!interfaceList)
	{
		//Failed to allocate memory
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}


	RtlZeroMemory(interfaceList, sizeof(USBD_INTERFACE_LIST_ENTRY));
	// 寻找配用接口描述符  格式化PUSBD_INTERFACE_LIST_ENTRY
	PUCHAR StartPosition = (PUCHAR)pdx->confDesc + pdx->confDesc->bLength;
	PUSB_INTERFACE_DESCRIPTOR temp = (PUSB_INTERFACE_DESCRIPTOR)StartPosition;
	int byteComsumed = pdx->confDesc->bLength;
	while (byteComsumed < pdx->confDesc->wTotalLength)
	{
		if (temp->bAlternateSetting == altIntNum)
		{
			interfaceDescriptor = temp;
			break;
		}
		ULONG size = sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR) * temp->bNumEndpoints;
		byteComsumed += size;
		StartPosition += size;
		temp = (PUSB_INTERFACE_DESCRIPTOR)StartPosition;
	}
	if (!interfaceDescriptor)
	{
		MyDbgPrint(("get interfacedesc failed"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}
	// Set the interface entry
	interfaceList->InterfaceDescriptor = interfaceDescriptor;
	interfaceList->Interface = NULL;


	// Allocate and build an URB for the select-configuration request.
	status = USBD_SelectInterfaceUrbAllocateAndBuild(
		pdx->UsbdHandle,
		pdx->confHandle,
		interfaceList,
		&urb);

	if (!NT_SUCCESS(status))
	{
		goto Exit;
	}

	status = SubmitUrbSync(pdx,	urb);

	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("set interface failed"));
		goto Exit;
	}

	// Enumerate the pipes in the interface information array, which is now filled with pipe
	// information.

	Interface = interfaceList->Interface;
	KIRQL oldIrql;
	ExAcquireFastMutex(&pdx->myMutex);
	pdx->pipeCount = Interface->NumberOfPipes;
	if (pdx->pipeInfos)
		ExFreePool(pdx->pipeInfos);
	pdx->pipeInfos = (PPIPE_INFO)ExAllocatePool(NonPagedPool, sizeof(PIPE_INFO) * pdx->pipeCount);
	ExReleaseFastMutex(&pdx->myMutex);

	if (!pdx->pipeInfos)
	{
		MyDbgPrint(("allocate pipeinfo failed"));
		goto Exit;
	}

	ExAcquireFastMutex(&pdx->myMutex);
	for (UCHAR i = 0; i < Interface->NumberOfPipes; i++)
	{
		pdx->pipeInfos[i].address = Interface->Pipes[i].EndpointAddress;
		pdx->pipeInfos[i].handle = Interface->Pipes[i].PipeHandle;
		pdx->pipeInfos[i].open = TRUE;
	}
	ExReleaseFastMutex(&pdx->myMutex);

Exit:

	if (interfaceList)
	{
		ExFreePool(interfaceList);
		interfaceList = NULL;
	}

	if (urb)
	{
		USBD_UrbFree(pdx->UsbdHandle, urb);
	}


	return status;

}


NTSTATUS SendNonEP0CtlData(PDEVICE_EXTENSION pdx, PIRP pIrp, UCHAR endAddress, PVOID isoInfoBuf, const ULONG isoLen, PVOID buffer, const ULONG bufLen)
{
	PURB urb = NULL;
	NTSTATUS  status;
	
	//构造URB
	USBD_PIPE_HANDLE handle = 0;
	for (UCHAR i = 0; i < pdx->pipeCount; i++)
	{
		if (pdx->pipeInfos[i].address == endAddress)
		{
			handle = pdx->pipeInfos[i].handle;
			break;
		}
	}
	if (!handle)
	{
		MyDbgPrint(("cannot find this point"));
		goto Exit;
	}


	ULONG TransferFlags = 0;
	if (USB_ENDPOINT_DIRECTION_IN(endAddress))
	{
		TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
	}
	if (isoLen)
	{
		status = USBD_IsochUrbAllocate(pdx->UsbdHandle, isoLen / sizeof(ISO_PACKET_INFO), &urb);
		if (!NT_SUCCESS(status))
		{
			MyDbgPrint(("USBD_IsochUrbAllocate faild"));
			goto Exit;
		}
	}
	else{
		status = USBD_UrbAllocate(pdx->UsbdHandle, &urb);
		if (!NT_SUCCESS(status))
		{
			MyDbgPrint(("USBD_UrbAllocate faild"));
			goto Exit;
		}
		UsbBuildInterruptOrBulkTransferRequest(urb, sizeof(_URB_BULK_OR_INTERRUPT_TRANSFER), handle, buffer, NULL, bufLen, TransferFlags, NULL);
	}


	//提交URB
	if (pIrp)
	{
		return SubmitUrbAsync(pdx, pIrp, urb);
	}
	else{
		status = SubmitUrbSync(pdx, urb);
		if (!NT_SUCCESS(status))
		{
			MyDbgPrint(("SendNonEP0Data failed"));
			goto Exit;
		}

		if (isoLen)
		{
			for (int i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++)
			{
				PISO_PACKET_INFO packetInfo = (PISO_PACKET_INFO)((PUCHAR)isoInfoBuf + sizeof(ISO_PACKET_INFO) * i);
				packetInfo->Length = urb->UrbIsochronousTransfer.IsoPacket[i].Length;
				packetInfo->Status = urb->UrbIsochronousTransfer.IsoPacket[i].Status;
			}
		}
	}
Exit:
	if (urb)
	{
		USBD_UrbFree(pdx->UsbdHandle, urb);
	}
	return status;
}


NTSTATUS VendorRequest(PDEVICE_EXTENSION pdx, PSINGLE_TRANSFER single)
{
	NTSTATUS status;

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	ULONG transflag = 0;
	if (single->SetupPacket.bmReqType.Direction == BMREQUEST_DEVICE_TO_HOST)
	{
		transflag = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
	}
	UsbBuildVendorRequest(urb, URB_FUNCTION_VENDOR_DEVICE,
		sizeof(_URB_CONTROL_VENDOR_OR_CLASS_REQUEST), 
		transflag, NULL,
		single->SetupPacket.bRequest, 
		single->SetupPacket.wValue, 
		single->SetupPacket.wIndex, 
		(PUCHAR)single + single->BufferOffset,
		NULL, single->BufferLength, NULL);
	status = SubmitUrbSync(pdx, urb);
	USBD_UrbFree(pdx->UsbdHandle, urb);
	return status;
}

VOID SetPwrComplete2(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus);
VOID QueryPwrComplete2(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus)
{
	MyDbgPrint(("enter QueryPwrComplete2"));
	if (IoStatus->Status == STATUS_SUCCESS)
	{
		MyDbgPrint(("success"));
		NTSTATUS status;
		KEVENT event2;
		KeInitializeEvent(&event2, NotificationEvent, FALSE);
		PoRequestPowerIrp(DeviceObject, IRP_MN_SET_POWER, PowerState, SetPwrComplete2, &event2, NULL);
	}
	PKEVENT event = (PKEVENT)Context;
	KeSetEvent(event, IO_NO_INCREMENT, FALSE);
	MyDbgPrint(("leave QueryPwrComplete2"));
}

VOID SetPwrComplete2(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus)
{
	MyDbgPrint(("enter SetPwrComplete2"));
	if (IoStatus->Status == STATUS_SUCCESS)
	{
		MyDbgPrint(("success"));
	}
	//PKEVENT event = (PKEVENT)Context;
	//KeSetEvent(event, IO_NO_INCREMENT, FALSE);
	MyDbgPrint(("leave SetPwrComplete2"));
}


NTSTATUS SetPwr(PDEVICE_EXTENSION pdx, POWER_STATE state)
{
	MyDbgPrint(("enter setpwr help"));
	NTSTATUS status;
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	status = PoRequestPowerIrp(pdx->NextStackDevice, IRP_MN_QUERY_POWER, state, QueryPwrComplete2, &event, NULL);
	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	}
	MyDbgPrint(("leave setpwr help"));
	return STATUS_SUCCESS;
}



NTSTATUS GetRegister(IN PCWSTR itemName, IN PCWSTR keyName, OUT PSTR value, IN ULONG len, OUT PULONG realSize)
{
	NTSTATUS status;
	HANDLE hReg;
	UNICODE_STRING regString;
	RtlInitUnicodeString(&regString, itemName);

	OBJECT_ATTRIBUTES objectAttributes;
	InitializeObjectAttributes(&objectAttributes, &regString, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&hReg, KEY_ALL_ACCESS, &objectAttributes);
	if (!NT_SUCCESS(status))
	{
		MyDbgPrint(("open reg failed"));
		return status;
	}


	UNICODE_STRING valuName;
	RtlInitUnicodeString(&valuName, keyName);
	status = ZwQueryValueKey(hReg, &valuName,
		KeyValuePartialInformation, NULL, len, realSize);

	if (status == STATUS_OBJECT_NAME_NOT_FOUND || *realSize == 0)
	{
		ZwClose(hReg);
		MyDbgPrint(("reg key not found"));
		return status;
	}

	PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)
		ExAllocatePool(PagedPool, *realSize);
	status = ZwQueryValueKey(hReg, &valuName,
		KeyValuePartialInformation, info, *realSize, realSize);
	if (!NT_SUCCESS(status))
	{
		ZwClose(hReg);
		MyDbgPrint(("read reg error"));
		return status;
	}

	for (int i = 0; i < *realSize; i += 2)
	{
		value[i / 2] = info->Data[i];
	}

	return status;
}


NTSTATUS AbortPipe(PDEVICE_EXTENSION pdx, UCHAR address)
{
	NTSTATUS status = -1;
	if (pdx->pipeCount == 0) {
		return STATUS_SUCCESS;
	}

	for (int i = 0; i < pdx->pipeCount; i++) {
		MyDbgPrint(("pipe address %d", pdx->pipeInfos[i].address));
		if (pdx->pipeInfos[i].address == address) {
			status = STATUS_SUCCESS;
			if (pdx->pipeInfos[i].open)
			{
				PURB urb;
				urb = (PURB)ExAllocatePool(NonPagedPool,
					sizeof(struct _URB_PIPE_REQUEST));

				if (urb) {
					urb->UrbHeader.Length = sizeof(struct _URB_PIPE_REQUEST);
					urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
					urb->UrbPipeRequest.PipeHandle = pdx->pipeInfos[i].handle;
					status = SubmitUrbSync(pdx, urb, FALSE);
					if (NT_SUCCESS(status)) {
						pdx->pipeInfos[i].open = FALSE;
					}
					ExFreePool(urb);
				}
				else {
					MyDbgPrint(("Failed to alloc urb\n"));
					status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			break;
		}
	}

	return status;
}


NTSTATUS ResetPipe(PDEVICE_EXTENSION pdx, UCHAR address)
{
	NTSTATUS status = -1;
	if (pdx->pipeCount == 0) {
		return STATUS_SUCCESS;
	}

	for (int i = 0; i < pdx->pipeCount; i++) {
		if (pdx->pipeInfos[i].address == address) {
			PURB urb;
			MyDbgPrint(("Aborting open pipe %d\n", i));
			urb = (PURB)ExAllocatePool(NonPagedPool,
				sizeof(struct _URB_PIPE_REQUEST));

			if (urb) {
				urb->UrbHeader.Length = sizeof(struct _URB_PIPE_REQUEST);
				urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
				urb->UrbPipeRequest.PipeHandle =
					pdx->pipeInfos[i].handle;
				status = SubmitUrbSync(pdx, urb, FALSE);
				if (NT_SUCCESS(status)) {
					pdx->pipeInfos[i].open = TRUE;
				}
				ExFreePool(urb);
			}
			else {
				MyDbgPrint(("Failed to alloc urb\n"));
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
			break;
		}
	}

	return status;
}


NTSTATUS ResetParentPort(PDEVICE_EXTENSION pdx)
{
	NTSTATUS           status;
	KEVENT             event;
	PIRP               irp;
	IO_STATUS_BLOCK    ioStatus;
	PIO_STACK_LOCATION nextStack;


	MyDbgPrint(("enter resetParentPort"));

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = IoBuildDeviceIoControlRequest(
		IOCTL_INTERNAL_USB_RESET_PORT,
		pdx->NextStackDevice,
		NULL,
		0,
		NULL,
		0,
		TRUE,
		&event,
		&ioStatus);

	if (NULL == irp) {

		MyDbgPrint(("memory alloc for irp failed"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	nextStack = IoGetNextIrpStackLocation(irp);

	ASSERT(nextStack != NULL);

	status = IoCallDriver(pdx->NextStackDevice, irp);

	if (STATUS_PENDING == status) {

		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	}
	else {

		ioStatus.Status = status;
	}

	status = ioStatus.Status;

	MyDbgPrint(("leave resetParentPort"));

	return status;

}