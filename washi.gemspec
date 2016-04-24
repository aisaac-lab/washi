# coding: utf-8
$:.push File.expand_path("../lib", __FILE__)
require 'washi/version'

Gem::Specification.new do |spec|
  spec.name          = "washi"
  spec.version       = Washi::VERSION
  spec.authors       = ["gogotanaka"]
  spec.email         = ["mail@tanakakazuki.com"]

  spec.summary       = %q{Slice deep-zoom images.}
  spec.description   = %q{Slice deep-zoom images.}
  spec.homepage      = "https://github.com/aisaac-lab/washi"
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split("\n")
  spec.test_files    = `git ls-files -- test/*`.split("\n")
  spec.bindir        = "exe"
  spec.executables   = "washi"
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler"
  spec.add_development_dependency "rake"
  spec.add_development_dependency "minitest", "~> 5.0"
end
