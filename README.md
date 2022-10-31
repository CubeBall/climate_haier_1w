# NOT READY YET!!!
# climate_haier_1w
Custom ESPHOME componets to control Haier AC with 1-wire central remote protocol 

# Example configuration entry
```yaml
external_components:
  - source: github://CubeBall/haier_1w

remote_receiver:
  id: rcvr
  pin:
    number: GPIO14
    inverted: true
    mode:
      input: true
      pullup: true
  # high 55% tolerance is recommended for some remote control units
  tolerance: 55%

remote_transmitter:
  id: trmr
  pin: GPIO32
  carrier_duty_percent: 100%

climate:
  - platform: climate_haier_1w       # adjust to match your AC unit!
    name: "AC"
    receiver_id: rcvr
    transmitter_id: trmr
```

## Additional configuration is available for this platform
### Configuration variables:

- header_send_high (Optional, Time): time for the high part of the header on Sending for the Haier protocol. Defaults to 200`000us
- header_receive_high (Optional, Time): time for the high part of the header on Receiving for the Haier protocol. Defaults to 32`000us
- header_send_low (Optional, Time): time for the low part of the header on sending for the Haier protocol. Defaults to 48`000us
- header_receive_low (Optional, Time): time for the low part of the header on receiving for the Haier protocol. Defaults to 24`000us
- bit_high (Optional, Time): time for the high part of any bit in the Haier protocol. Defaults to 4`000us
- bit_one_low (Optional, Time): time for the low part of a ‘1’ bit in the Haier protocol. Defaults to 12`000us
- bit_zero_low (Optional, Time): time for the low part of a ‘0’ bit in the Haier protocol. Defaults to 4`000us
