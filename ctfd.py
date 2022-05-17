#!/usr/bin/python3

import json, pandas, requests, secrets, string, sys
from pprint import pprint

url = 'DEMO-CTFD-URL'
token = 'DEMOTOKEN'

# Enable logging
'''
import logging 

# These two lines enable debugging at httplib level (requests->urllib3->http.client)
# You will see the REQUEST, including HEADERS and DATA, and RESPONSE with HEADERS but without DATA.
# The only thing missing will be the response.body which is not logged.
try:
    import http.client as http_client
except ImportError:
    # Python 2
    import httplib as http_client
http_client.HTTPConnection.debuglevel = 1

# You must initialize logging, otherwise you'll not see debug output.
logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)
requests_log = logging.getLogger("requests.packages.urllib3")
requests_log.setLevel(logging.DEBUG)
requests_log.propagate = True
'''


class ctfd:

	def __init__(self, url, token):

		self.url = url
		self.token = token
		self.session = self.generate_session()
		self.users = []
		self.teams = []


	def generate_session(self):

		session = requests.Session()

		session.headers.update({"Authorization": f"Token {self.token}"})
		return session


	def generate_password(self):

		alphabets = string.ascii_letters + string.digits + '!@#$%^&*'

		password = ''.join(secrets.choice(alphabets) for i in range(16))
		return password


	def save_passwords(self):

		users = pandas.DataFrame(self.users)
		users.rename(columns={0: 'name', 1: 'email', 2: 'password'}, inplace=True)
		users.to_csv('users.csv', index=False)

		teams = pandas.DataFrame(self.teams)
		teams.rename(columns={0: 'name', 1: 'email', 2: 'password'}, inplace=True)
		teams.to_csv('teams.csv', index=False)


	def register_user(self, name, email):

		verified = True
		password = self.generate_password()

		request_url = f"{self.url}/api/v1/users"
		data = {
			'name': name,
			'email': email,
			'password': password,
			'verified': verified,
			}

		response = self.session.post(request_url, json=data).json()

		if response['success']:
			print(f'[+] register_user(name={name}, email={email}) succeded')
			self.users.append((name, email, password))
			return response['data']['id']
		else:
			print(f'[-] register_user(name={name}, email={email}) failed')
			self.save_passwords()
			sys.exit(1)


	def register_team(self, name, email):

		password = self.generate_password()

		request_url = f"{self.url}/api/v1/teams"
		data = {
			'name': name,
			'email': email,
			'password': password,
			}

		response = self.session.post(request_url, json=data).json()

		if response['success']:
			print(f'[+] register_team(name={name}, email={email}) succeded')
			self.teams.append((name, email, password))
			return response['data']['id']
		else:
			print(f'[-] register_team(name={name}, email={email}) failed')
			self.save_passwords()
			sys.exit(2)


	def add_member(self, team_id, user_id):

		request_url = f"{self.url}/api/v1/teams/{team_id}/members"
		data = {
			'user_id': user_id,
			}

		response = self.session.post(request_url, json=data).json()

		if response['success']:
			print(f'[+] add_member(team_id={team_id}, user_id={user_id}) succeded')
		else:
			print(f'[-] add_member(team_id={team_id}, user_id={user_id}) failed')
			self.save_passwords()
			sys.exit(3)


	def make_captain(self, team_id, user_id):

		request_url = f"{self.url}/api/v1/teams/{team_id}"
		data = {
			'captain_id': user_id
			}

		response = self.session.patch(request_url, json=data).json()

		if response['success']:
			print(f'[+] make_captain(team_id={team_id}, user_id={user_id}) succeded')
		else:
			print(f'[-] make_captain(team_id={team_id}, user_id={user_id}) failed')
			self.save_passwords()
			sys.exit(4)
  


def main():

	df = pandas.read_csv(sys.argv[1])

	zion = ctfd(url, token)

	print()
	for i, team in df.iterrows():

		teamName = team['teamName']

		captain = json.loads(team['leader'])
		captainName = captain['Name']
		captainEmail = captain['Email']

		users = []
		members = json.loads(team['teamMembers'])
		for member in members:
			user = json.loads(member)
			userName = user['Name']
			userEmail = user['Email']
			users.append((userName, userEmail))

		captainId = zion.register_user(captainName, captainEmail)
		teamId = zion.register_team(teamName, captainEmail)
		zion.add_member(teamId, captainId)
		zion.make_captain(teamId, captainId)

		for userName, userEmail in users:
			userId = zion.register_user(userName, userEmail)
			zion.add_member(teamId, userId)

		print()

	zion.save_passwords()

	print('Goodbye')



if __name__ == '__main__':
	if len(sys.argv) >= 2:
		main()
	else:
		print("Usage: python3 ctfd.py <csv_file>")
