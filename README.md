포트폴리오는 src 폴더 하위 파일들을 참고 해주시면 됩니다.

install mysql
https://dev.mysql.com/downloads/windows/installer/8.0.html
mysql-installer-community-8.0.23.0
MySQL Server x64
Connector/C++ x64

cp C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.dll _build
cp C:/Program Files/MySQL/Connector C++ 8.0/lib64/*.dll _build


mkdir _build
cd _build
cmake -A x64 ..
