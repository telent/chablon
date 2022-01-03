(import-macros {: offset} :grovel.macros)

(fn a-thing []
  (print (offset "ble-constants.gen.json" :ble_gatt_svc_def :uuid)))

(comment
(local uuid (require :ble.uuid))
(local chr (require :ble.gatt.chr))
(local defs (require :defs.ble.gatt.svc))

(fn initialize [ud uuid characteristics]
  (unsafe.poke ud
               defs.fields.type.offset
               defs.constants.BLE_GATT_SVC_TYPE_PRIMARY
               defs.fields.type.size)
  (uuid.initialize (unsafe.ud+ ud defs.fields.uuid.offset) uuid)
  (let [chrs (unsafe.alloc (* (# characteristics) ble.gatt.chr.size))]
    (for [i v (ipairs characteristics)]
      (chr.initialize (unsafe.ud+
                       chrs (* (- i 1) ble.gatt.chr.size))
                      v))
    (unsafe.poke ud defs.fields.characteristics.offset 4 chrs))
  ud)
)
