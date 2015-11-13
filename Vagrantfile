# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # If true, then any SSH connections made will enable agent forwarding.
  # Default value: false
  # config.ssh.forward_agent = true

  # Enable provisioning with Puppet stand alone.  Puppet manifests
  # are contained in a directory path relative to this Vagrantfile.
  # You will need to create the manifests directory and a manifest in
  # the file default.pp in the manifests_path directory.
  #

  config.vm.define "trusty64" do |trusty64|
    trusty64.vm.box = "ubuntu/trusty64"

    trusty64.vm.provider "virtualbox" do |v|
      v.memory = 16*1024
      v.cpus = 2
    end
    trusty64.vm.provision :shell, path: "vagrant/provision-debian.sh"
  end

  config.vm.define "jessie64" do |jessie64|
    jessie64.vm.box = "debian/jessie64"

    jessie64.vm.provider "virtualbox" do |v|
      v.memory = 16*1024
      v.cpus = 2
    end
    jessie64.vm.provision :shell, path: "vagrant/provision-debian.sh"
  end

  # config.vm.define "trusty32" do |trusty32|
  #   trusty32.vm.box = "ubuntu/trusty32"
  #   #trusty32.vm.autostart = false

  #   trusty32.vm.provision :shell, path: "vagrant/provision-trusty.sh"
  # end

  # config.vm.define "freebsd64" do |freebsd64|
  #   freebsd64.vm.box = "chef/freebsd-10.0"
  #   #freebsd64.vm.autostart = false

  #   freebsd64.vm.provision :shell, path: "vagrant/provision-freebsd.sh"
  # end

  # config.vm.define "freebsd32" do |freebsd32|
  #   freebsd32.vm.box = "chef/freebsd-9.2-i386"
  #   #freebsd32.vm.autostart = false

  #   freebsd32.vm.provision :shell, path: "vagrant/provision-freebsd.sh"
  # end

  # config.vm.define "win7" do |win7|
  #   win7.vm.box = "http://aka.ms/vagrant-win7-ie11"
  #   win7.vm.autostart = false
  # end
end
