set -e
set -x

tsx ../../../run.ts uapi.json

echo "start JS Test"
npm run test
