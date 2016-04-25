
#include "usbdriver.h"


#pragma PAGEDCODE
NTSTATUS DispatchPower(IN PDEVICE_OBJECT pFdo, IN PIRP pIrp)
{
	PAGED_CODE();
	MyDbgPrint((" Enter Power"));
	DisplayProcessName();


	NTSTATUS status = -1;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pFdo->DeviceExtension;
	switch (stack->MinorFunction)
	{
	case IRP_MN_SET_POWER:
		switch (stack->Parameters.Power.Type)
		{
		case SystemPowerState:
			SystemSetPowerHandler(pdx, pIrp);
			status = STATUS_PENDING;
			break;
		case DevicePowerState:
			DeviceSetPowerHandler(pdx, pIrp);
			break;
		}
		break;
	case IRP_MN_QUERY_POWER:
		switch (stack->Parameters.Power.Type)
		{
		case SystemPowerState:
			SystemQueryPowerHandler(pdx, pIrp);
			break;
		case DevicePowerState:
			DeviceQueryPowerHandler(pdx, pIrp);
			break;
		}
		break;
	case IRP_MN_WAIT_WAKE:
		break;
	default:
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(pdx->NextStackDevice, pIrp);

		if (!NT_SUCCESS(status)) {
			MyDbgPrint(("Lower drivers failed this Irp"));
		}
		break;
	}

	MyDbgPrint((" Leave Power"));
	return status;
}

VOID QueryPwrComplete(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus)
{
	MyDbgPrint(("Enter QueryPwrComplete"));
	DisplayProcessName();

	PIRP sysIrp = (PIRP)Context;
	sysIrp->IoStatus.Status = IoStatus->Status;
	IoCompleteRequest(sysIrp, IO_NO_INCREMENT);

	MyDbgPrint(("leave QueryPwrComplete"));
}


NTSTATUS OnQueryRequestComplete(PDEVICE_OBJECT pdo, PIRP pIrp, PVOID context)
{
	MyDbgPrint(("enter OnQueryRequestComplete"));
	DisplayProcessName();

	NTSTATUS status = pIrp->IoStatus.Status;
	if (!NT_SUCCESS(status))
	{
		return STATUS_SUCCESS;
	}
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	POWER_STATE state;
	if (stack->Parameters.Power.State.SystemState == PowerSystemWorking)
		state.DeviceState = PowerDeviceD0;
	else
		state.DeviceState = PowerDeviceD3;
	PoRequestPowerIrp(pdo, IRP_MN_QUERY_POWER, state, QueryPwrComplete, pIrp, NULL);
	MyDbgPrint(("leave OnQueryRequestComplete"));
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS SystemQueryPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp)
{
	MyDbgPrint(("Enter SystemQueryPowerHandler"));
	DisplayProcessName();

	IoMarkIrpPending(pIrp);
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, OnQueryRequestComplete, NULL, TRUE, TRUE, TRUE);

	IoCallDriver(pdx->NextStackDevice, pIrp);
	MyDbgPrint(("Enter SystemQueryPowerHandler"));
	return STATUS_PENDING;
}

VOID SetPwrComplete(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PVOID Context, PIO_STATUS_BLOCK IoStatus)
{
	MyDbgPrint(("enter SetPwrComplete"));
	DisplayProcessName();

	if (NT_SUCCESS(IoStatus->Status))
	{
		PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
		pdx->pwrState = PowerState.DeviceState;
	}
	PIRP sysIrp = (PIRP)Context;
	sysIrp->IoStatus.Status = IoStatus->Status;
	IoCompleteRequest(sysIrp, IO_NO_INCREMENT);

	MyDbgPrint(("leave SetPwrComplete"));
}

NTSTATUS OnSetRequestComplete(PDEVICE_OBJECT pdo, PIRP pIrp, PVOID context)
{
	MyDbgPrint(("enter OnSetRequestComplete"));
	DisplayProcessName();

	NTSTATUS status = pIrp->IoStatus.Status;
	if (!NT_SUCCESS(status))
	{
		return STATUS_SUCCESS;
	}
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	POWER_STATE state;
	if (stack->Parameters.Power.State.SystemState == PowerSystemWorking)
		state.DeviceState = PowerDeviceD0;
	else
		state.DeviceState = PowerDeviceD3;
	PoRequestPowerIrp(pdo, IRP_MN_QUERY_POWER, state, SetPwrComplete, pIrp, NULL);
	MyDbgPrint(("leave OnSetRequestComplete"));
	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS SystemSetPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp)
{
	MyDbgPrint(("Enter SystemSetPowerHandler"));
	DisplayProcessName();
	IoMarkIrpPending(pIrp);
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, OnSetRequestComplete, NULL, TRUE, TRUE, TRUE);

	IoCallDriver(pdx->NextStackDevice, pIrp);
	MyDbgPrint(("leave SystemSetPowerHandler"));
	return STATUS_PENDING;


}







NTSTATUS DeviceQueryPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp)
{
	MyDbgPrint(("Enter DeviceQueryPowerHandler"));
	DisplayProcessName();
	IoMarkIrpPending(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	IoCallDriver(pdx->NextStackDevice, pIrp);
	MyDbgPrint(("leave DeviceQueryPowerHandler"));
	return STATUS_PENDING;
}


NTSTATUS OnDeviceSetComplete(PDEVICE_OBJECT pdo, PIRP pIrp, PVOID context)
{
	MyDbgPrint(("enter OnDeviceSetComplete"));
	DisplayProcessName();

	NTSTATUS status = pIrp->IoStatus.Status;
	if (NT_SUCCESS(status))
	{
		MyDbgPrint(("OnDeviceSetComplete success"));
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
		PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pdo->DeviceExtension;
		pdx->pwrState = stack->Parameters.Power.State.DeviceState;
	}

	MyDbgPrint(("leave OnDeviceSetComplete"));
	return STATUS_SUCCESS;
}

NTSTATUS DeviceSetPowerHandler(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp)
{
	MyDbgPrint(("Enter DeviceSetPowerHandler"));
	DisplayProcessName();

	IoMarkIrpPending(pIrp);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, OnDeviceSetComplete, NULL, TRUE, TRUE, TRUE);
	IoCallDriver(pdx->NextStackDevice, pIrp);

	MyDbgPrint(("leave DeviceSetPowerHandler"));
	return STATUS_PENDING;
}