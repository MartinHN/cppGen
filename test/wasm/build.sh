set -e
set -x

ts-node ../../../run.ts uapi.json

echo "start JS Test"
npm run test
