import { DeviceCheckin } from '@teamkeel/sdk'

export default DeviceCheckin(async (inputs, api) => {
  // Build something awesome

  
  const { name, staff, patient, device } = inputs

  return {
  name: name,
  staff: staff,
  patient: patient,
  device: device
  }
})