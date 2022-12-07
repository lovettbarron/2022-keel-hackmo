import { DeviceCheckin, Checkin } from '@teamkeel/sdk'

export default DeviceCheckin(async (inputs, api) => {
  // Build something awesome

  const { name, staffId, patientId, deviceId } = inputs

  const values: Partial<Checkin> = {
    ...inputs.values,
  };

  return api.models.checkin.create({
    ...values,
  });
})