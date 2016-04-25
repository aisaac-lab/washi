module Washil
  @@config = Class.new {
    attr_accessor :overlap, :binapath, :options

    def initialize
      @overlap  = 0
      @binapath = 'vips dzsave'
      @options  = %w|--vips-progress --vips-leak|
    end
  }.new

  def self.config
    @@config
  end
end
