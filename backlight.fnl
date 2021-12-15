(let [high 23
      medium 22
      low 14]
  {
   :init (fn []
           (gpio.set_direction high 0)
           (gpio.set_direction medium 0)
           (gpio.set_direction low 0))
   :set_brightness (fn [level]
                     (gpio.write high (if (>= level 3) 0 1))
                     (gpio.write medium (if (>= level 2) 0 1))
                     (gpio.write low (if (>= level 1) 0 1)))
   })
