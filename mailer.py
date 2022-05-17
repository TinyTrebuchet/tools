#!/usr/bin/python3

import smtplib, ssl, sys
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from pandas import read_csv


SENDER_MAIL = "<MAIL-ID>"
SENDER_PASSWORD = "<MAIL-APP-PASSWORD>"

template = """\
Hello {name},

Your account has been registered on zionctf.iitmandi.co.in

Please use the following credentials to login during the CTF:
  email: {email}
  password: {password}

"""

subject = "Login credentials for ZionCTF"


if len(sys.argv) < 2:
	print("Usage: python3 mailer.py <csv_file>")



df = read_csv(sys.argv[1])

context = ssl.create_default_context()

print()
with smtplib.SMTP_SSL('smtp.gmail.com', 465, context=context) as server:

	server.login(SENDER_MAIL, SENDER_PASSWORD)

	for idx, row in df.iterrows():

		name, email, password = row

		msg = MIMEMultipart()

		msg['From'] = SENDER_MAIL
		msg['To'] = email
		msg['Subject'] = subject

		message = template.format(name=name, email=email, password=password)

		msg.attach(MIMEText(message))

		server.sendmail(SENDER_MAIL, email, msg.as_string())

		print(f'[+] Mail sent to {email}')

print('\nGoodbye')
