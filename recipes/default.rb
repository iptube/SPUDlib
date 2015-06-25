include_recipe "cmake-bin"
package "g++"

dir = "/home/vagrant/dev/#{`hostname`.chop()}"

directory dir do
  action :create
end

execute "run cmake" do
  cwd dir
  command "cmake .."
  creates "#{dir}/Makefile"
end

execute "run make" do
  cwd dir
  command "make"
end
