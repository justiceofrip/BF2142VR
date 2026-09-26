"""Run in authenticated AWS CloudShell after creating the one-month test instance.
Creates deletion schedules restricted to that exact instance ARN. No credentials
are stored in the game or join package. Does not create or resize paid instances.
"""
import argparse, datetime, json, subprocess, time

def aws(*args):
    result = subprocess.run(['aws', *args, '--output', 'json', '--no-cli-pager'], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr.strip())
    return json.loads(result.stdout or '{}')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--region',required=True)
    parser.add_argument('--instance',default='bf2142-vr-test')
    parser.add_argument('--days',type=int,default=29,choices=range(1,30))
    a=parser.parse_args()
    if not all(c.isalnum() or c=='-' for c in a.instance+a.region): raise ValueError('Invalid instance/region')
    instance=aws('lightsail','get-instance','--region',a.region,'--instance-name',a.instance)['instance']
    bundles=aws('lightsail','get-bundles','--region',a.region)['bundles']
    bundle=next(x for x in bundles if x['bundleId']==instance['bundleId'])
    if bundle['price']>25 or 'WINDOWS' not in bundle.get('supportedPlatforms',[]) or not instance['blueprintId'].startswith('windows_server_'): raise ValueError('Expected the Windows test instance within the agreed $25 monthly base budget')
    created=datetime.datetime.fromisoformat(str(instance['createdAt']).replace('Z','+00:00'))
    now=datetime.datetime.now(datetime.timezone.utc)
    expiry=created+datetime.timedelta(days=a.days)
    if expiry<=now: raise ValueError('The intended expiry has already passed; delete the instance in Lightsail now')
    account=aws('sts','get-caller-identity')['Account'];arn=instance['arn']
    role_name='BF2142VR-Expire-'+a.instance
    role_arn='arn:aws:iam::'+account+':role/'+role_name
    trust={'Version':'2012-10-17','Statement':[{'Effect':'Allow','Principal':{'Service':'scheduler.amazonaws.com'},'Action':'sts:AssumeRole','Condition':{'StringEquals':{'aws:SourceAccount':account},'ArnEquals':{'aws:SourceArn':'arn:aws:scheduler:'+a.region+':'+account+':schedule-group/default'}}}]}
    # This role can delete only this exact instance, not other AWS resources.
    policy={'Version':'2012-10-17','Statement':[{'Effect':'Allow','Action':'lightsail:DeleteInstance','Resource':arn}]}
    try: role=aws('iam','get-role','--role-name',role_name)['Role']
    except RuntimeError as e:
        if 'NoSuchEntity' not in str(e): raise
        role=aws('iam','create-role','--role-name',role_name,'--assume-role-policy-document',json.dumps(trust),'--description','BF2142VR one-month test expiry','--tags',json.dumps([{'Key':'BF2142VR-InstanceArn','Value':arn}]))['Role']
    tags=aws('iam','list-role-tags','--role-name',role_name).get('Tags',[])
    if not any(t['Key']=='BF2142VR-InstanceArn' and t['Value']==arn for t in tags): raise ValueError('An unrelated role already uses this name; it was left unchanged')
    aws('iam','put-role-policy','--role-name',role_name,'--policy-name','DeleteOnlyThisTestInstance','--policy-document',json.dumps(policy))
    target={'Arn':'arn:aws:scheduler:::aws-sdk:lightsail:deleteInstance','RoleArn':role_arn,'Input':json.dumps({'InstanceName':a.instance,'ForceDeleteAddOns':True}),'RetryPolicy':{'MaximumEventAgeInSeconds':3600,'MaximumRetryAttempts':5}}
    for suffix,when in [('',expiry),('-backup',expiry+datetime.timedelta(hours=12))]:
        name=a.instance+'-expiry'+suffix
        try:
            existing=aws('scheduler','get-schedule','--region',a.region,'--name',name)
            if existing['Target']['Input']!=target['Input'] or existing['Target']['RoleArn']!=role_arn: raise ValueError('An unrelated schedule uses this name')
            print('Existing deletion retained:',existing['ScheduleExpression']);continue
        except RuntimeError as e:
            if 'ResourceNotFoundException' not in str(e): raise
        expression='at('+when.astimezone(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%S')+')'
        for attempt in range(6):
            try:
                aws('scheduler','create-schedule','--region',a.region,'--name',name,'--schedule-expression',expression,'--schedule-expression-timezone','UTC','--flexible-time-window','{"Mode":"OFF"}','--action-after-completion','DELETE','--target',json.dumps(target));break
            except RuntimeError as e:
                if attempt==5 or 'assume' not in str(e).lower():raise
                time.sleep(5)
        check=aws('scheduler','get-schedule','--region',a.region,'--name',name)
        if check['State']!='ENABLED' or check['ScheduleExpression']!=expression: raise ValueError('Deletion schedule verification failed')
        print('Verified automatic deletion:',a.instance,expression,'UTC')
    print('Base plan: $'+str(bundle['price'])+'/month. Taxes and usage overages are separate. No snapshots/static IPs were created by this script.')
    print('Deletion also removes the server disk. Keep mod source and any wanted server configuration on your own PC.')
if __name__=='__main__':main()
