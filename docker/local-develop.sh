#!/bin/bash

function dependency(){
	(command -v $1 > /dev/null || (echo "install $1" && exit 1))
}

dependency jq     || exit 1
dependency docker || exit 2
dependency grep   || exit 3
dependency sed    || exit 4

# usage
echo "execute 'source $0' to get all env vars for testing"

# cleanup
docker rm -f pg rd 2>/dev/null

# vars
user=iroha
password=iroha

# run postgres and redis
pgid=$(docker run --rm --name pg -e POSTGRES_USER=$user -e POSTGRES_PASSSWORD=$password -d postgres:9.5)
rdid=$(docker run --rm --name rd -d redis:3.2.8)

pgip=$(docker inspect $pgid | jq .[0].NetworkSettings.IPAddress | sed "s/\"//g")
rdip=$(docker inspect $rdid | jq .[0].NetworkSettings.IPAddress | sed "s/\"//g")

echo "postgres: $pgip:5432 {login: $user, pwd: $password}"
echo "redis:    $rdip:6379"

export IROHA_POSTGRES_HOST=$pgip
export IROHA_POSTGRES_PORT=5432
export IROHA_POSTGRES_USER=$user
export IROHA_POSTGRES_PASSWORD=$password
export IROHA_REDIS_HOST=$rgip
export IROHA_REDIS_PORT=6379