#!/bin/bash

# Launch dev docker services
pushd ../../docker/dev
docker-compose up -d
popd

# Grant all privileges.
mysql --protocol=tcp -u root -pdev123$ mysql -e "GRANT ALL PRIVILEGES ON *.* TO 'lac_dev'@'%' WITH GRANT OPTION;"