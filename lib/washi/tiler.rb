require 'washi/config'

module Washi
  class Tiler
    def self.call(input_path, output_path, options={})
      unless tile_size = options[:tile_size]
        fail "Need pass :tile_size"
      end
      `#{Washil.config.binapath} #{input_path} #{output_path} --tile-size #{tile_size} --overlap #{Washil.config.overlap} #{Washil.config.options.join(" ")}`
    end
  end
end
