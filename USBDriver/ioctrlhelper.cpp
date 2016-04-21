

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
	if (pdx->deviceDesc)
	{
		ExFreePool(pdx->deviceDesc);
	}

	pdx->deviceDesc = (PUSB_DEVICE_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeof(USB_DEVICE_DESCRIPTOR));
	if (!pdx->deviceDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	UsbBuildGetDescriptorRequest(urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, pdx->deviceDesc, NULL, sizeof(USB_DEVICE_DESCRIPTOR), NULL);
	status = SubmitUrbSync(pdx, urb);
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
	if (pdx->confDesc)
	{
		ExFreePool(pdx->confDesc);
	}

	//第一步获取大小
	pdx->confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeof(USB_CONFIGURATION_DESCRIPTOR));
	if (!pdx->confDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	PURB urb;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb);
	UsbBuildGetDescriptorRequest(urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pdx->confDesc, NULL, sizeof(USB_CONFIGURATION_DESCRIPTOR), NULL);
	status = SubmitUrbSync(pdx, urb);
	USBD_UrbFree(pdx->UsbdHandle, urb);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	//第二步获取完整的
	ULONG size = pdx->confDesc->wTotalLength;
	ExFreePool(pdx->confDesc);
	pdx->confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, size);
	if (!pdx->confDesc)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	PURB urb2;
	USBD_UrbAllocate(pdx->UsbdHandle, &urb2);
	UsbBuildGetDescriptorRequest(urb2, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
		USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, pdx->confDesc, NULL, size, NULL);
	status = SubmitUrbSync(pdx, urb2);
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
		//interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
		//	pdx->confDesc,
		//	StartPosition, // StartPosition 
		//	-1,            // InterfaceNumber
		//	0,             // AlternateSetting
		//	-1,            // InterfaceClass
		//	-1,            // InterfaceSubClass
		//	-1);           // InterfaceProtocol

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
		pdx->pipeCount = Interface->NumberOfPipes;
		if (pdx->pipeInfos)
			ExFreePool(pdx->pipeInfos);
		pdx->pipeInfos = (PPIPE_INFO)ExAllocatePool(NonPagedPool, sizeof(PIPE_INFO) * pdx->pipeCount);
		if (!pdx->pipeInfos)
		{
			MyDbgPrint(("allocate pipeinfo failed"));
			goto Exit;
		}

		for (i = 0; i < Interface->NumberOfPipes; i++)
		{
			pdx->pipeInfos[i].address = Interface->Pipes[i].EndpointAddress;
			pdx->pipeInfos[i].handle = Interface->Pipes[i].PipeHandle;
			//if (Interface->Pipes[i].PipeType == UsbdPipeTypeInterrupt)
			//{
			//	pdx->InterruptPipe = pipeHandle;
			//}
			//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_IN(Interface->Pipes[i].EndpointAddress))
			//{
			//	pdx->BulkInPipe = pipeHandle;
			//}
			//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_OUT(Interface->Pipes[i].EndpointAddress))
			//{
			//	pdx->BulkOutPipe = pipeHandle;
			//}
		}
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
	//interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
	//	pdx->confDesc,
	//	StartPosition, // StartPosition 
	//	0,            // InterfaceNumber
	//	altIntNum,    // AlternateSetting
	//	-1,            // InterfaceClass
	//	-1,            // InterfaceSubClass
	//	-1);           // InterfaceProtocol
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
	pdx->pipeCount = Interface->NumberOfPipes;
	if (pdx->pipeInfos)
		ExFreePool(pdx->pipeInfos);
	pdx->pipeInfos = (PPIPE_INFO)ExAllocatePool(NonPagedPool, sizeof(PIPE_INFO) * pdx->pipeCount);
	if (!pdx->pipeInfos)
	{
		MyDbgPrint(("allocate pipeinfo failed"));
		goto Exit;
	}
	for (UCHAR i = 0; i < Interface->NumberOfPipes; i++)
	{
		pdx->pipeInfos[i].address = Interface->Pipes[i].EndpointAddress;
		pdx->pipeInfos[i].handle = Interface->Pipes[i].PipeHandle;
		//if (Interface->Pipes[i].PipeType == UsbdPipeTypeInterrupt)
		//{
		//	pdx->InterruptPipe = pipeHandle;
		//}
		//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_IN(Interface->Pipes[i].EndpointAddress))
		//{
		//	pdx->BulkInPipe = pipeHandle;
		//}
		//if (Interface->Pipes[i].PipeType == UsbdPipeTypeBulk && USB_ENDPOINT_DIRECTION_OUT(Interface->Pipes[i].EndpointAddress))
		//{
		//	pdx->BulkOutPipe = pipeHandle;
		//}
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


VOID PwrComplete(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus)
{
	
	if (IoStatus->Status == STATUS_SUCCESS && MinorFunction == IRP_MN_QUERY_POWER)
	{
		NTSTATUS status;
		KEVENT event;
		KeInitializeEvent(&event, NotificationEvent, FALSE);
		status = PoRequestPowerIrp(DeviceObject, IRP_MN_SET_POWER, PowerState, PwrComplete, &event, NULL);
		if (status == STATUS_PENDING)
		{
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		}
	}
	PKEVENT event = (PKEVENT)Context;
	KeSetEvent(event, IO_NO_INCREMENT, FALSE);
}


NTSTATUS SetPwr(PDEVICE_EXTENSION pdx, POWER_STATE state)
{
	NTSTATUS status;
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	status = PoRequestPowerIrp(pdx->fdo, IRP_MN_QUERY_POWER, state, PwrComplete, &event, NULL);
	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	}
	return STATUS_SUCCESS;
}