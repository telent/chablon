(local buffer-length 10)

(fn new-spi [instance params]
  (let [buffer (byte_buffer.new buffer-length)
        handle (spictl_ffi.new instance params)]
    {
     :transfer (fn [spi payload count]
                 (let [buf (byte_buffer.from_table buffer payload)
                       len (or count (# payload)) ]
                   (spictl_ffi.transfer handle buf len)
                   ))
     :transfer_raw (fn [spi payload count]
                     (spictl_ffi.transfer handle payload count))
     }))

{
 :new new-spi
 }
