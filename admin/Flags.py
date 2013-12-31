# Cheroke Admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

import CTK

ISO_3166 = {
    "AD" : "Andorra",
    "AE" : "United Arab Emirates",
    "AF" : "Afghanistan",
    "AG" : "Antigua and Barbuda",
    "AI" : "Anguilla",
    "AL" : "Albania",
    "AM" : "Armenia",
    "AN" : "Netherlands Antilles",
    "AO" : "Angola",
    "AR" : "Argentina",
    "AS" : "American Samoa",
    "AT" : "Austria",
    "AU" : "Australia",
    "AW" : "Aruba",
    "AX" : "Aland Islands",
    "AZ" : "Azerbaijan",
    "BA" : "Bosnia & Herzegovina",
    "BB" : "Barbados",
    "BD" : "Bangladesh",
    "BE" : "Belgium",
    "BF" : "Burkina Faso",
    "BG" : "Bulgaria",
    "BH" : "Bahrain",
    "BJ" : "Benin",
    "BM" : "Bermuda",
    "BN" : "Brunei Darussalam",
    "BO" : "Bolivia",
    "BR" : "Brazil",
    "BS" : "Bahamas",
    "BT" : "Bhutan",
    "BV" : "Bouvet Island",
    "BW" : "Botswana",
    "BY" : "Belarus",
    "BZ" : "Belize",
    "CA" : "Canada",
    "CC" : "Cocos (Keeling) Islands",
    "CD" : "Republic of the Congo",
    "CF" : "Central African Republic",
    "CG" : "Congo",
    "CH" : "Switzerland",
    "CI" : "Cote D'Ivoire",
    "CK" : "Cook Islands",
    "CL" : "Chile",
    "CM" : "Cameroon",
    "CN" : "China",
    "CO" : "Colombia",
    "CR" : "Costa Rica",
    "CU" : "Cuba",
    "CV" : "Cape Verde",
    "CX" : "Christmas Island",
    "CY" : "Cyprus",
    "CZ" : "Czech Republic",
    "DE" : "Germany",
    "DJ" : "Djibouti",
    "DK" : "Denmark",
    "DM" : "Dominica",
    "DO" : "Dominican Republic",
    "DZ" : "Algeria",
    "EC" : "Ecuador",
    "EE" : "Estonia",
    "EG" : "Egypt",
    "EH" : "Western Sahara",
    "ER" : "Eritrea",
    "ES" : "Spain",
    "ET" : "Ethiopia",
    "FI" : "Finland",
    "FJ" : "Fiji",
    "FK" : "Malvinas",
    "FM" : "Micronesia, States of",
    "FO" : "Faroe Islands",
    "FR" : "France",
    "GA" : "Gabon",
    "GB" : "United Kingdom",
    "GD" : "Grenada",
    "GE" : "Georgia",
    "GF" : "French Guiana",
    "GH" : "Ghana",
    "GI" : "Gibraltar",
    "GL" : "Greenland",
    "GM" : "Gambia",
    "GN" : "Guinea",
    "GP" : "Guadeloupe",
    "GQ" : "Equatorial Guinea",
    "GR" : "Greece",
    "GS" : "South Georgia",
    "GT" : "Guatemala",
    "GU" : "Guam",
    "GW" : "Guinea-Bissau",
    "GY" : "Guyana",
    "HK" : "Hong Kong",
    "HM" : "Heard & McDonald",
    "HN" : "Honduras",
    "HR" : "Croatia",
    "HT" : "Haiti",
    "HU" : "Hungary",
    "ID" : "Indonesia",
    "IE" : "Ireland",
    "IL" : "Israel",
    "IN" : "India",
    "IO" : "British Indian",
    "IQ" : "Iraq",
    "IR" : "Iran, Republic of",
    "IS" : "Iceland",
    "IT" : "Italy",
    "JM" : "Jamaica",
    "JO" : "Jordan",
    "JP" : "Japan",
    "KE" : "Kenya",
    "KG" : "Kyrgyzstan",
    "KH" : "Cambodia",
    "KI" : "Kiribati",
    "KM" : "Comoros",
    "KN" : "Saint Kitts and Nevis",
    "KP" : "Korea, Republic of",
    "KR" : "Korea, Republic of",
    "KW" : "Kuwait",
    "KY" : "Cayman Islands",
    "KZ" : "Kazakhstan",
    "LA" : "Lao, Republic",
    "LB" : "Lebanon",
    "LC" : "Saint Lucia",
    "LI" : "Liechtenstein",
    "LK" : "Sri Lanka",
    "LR" : "Liberia",
    "LS" : "Lesotho",
    "LT" : "Lithuania",
    "LU" : "Luxembourg",
    "LV" : "Latvia",
    "LY" : "Libyan Arab Jamahiriya",
    "MA" : "Morocco",
    "MC" : "Monaco",
    "MD" : "Moldova, Republic of",
    "ME" : "Montenegro",
    "MG" : "Madagascar",
    "MH" : "Marshall Islands",
    "MK" : "Macedonia",
    "ML" : "Mali",
    "MM" : "Myanmar",
    "MN" : "Mongolia",
    "MO" : "Macau",
    "MP" : "N. Mariana Islands",
    "MQ" : "Martinique",
    "MR" : "Mauritania",
    "MS" : "Montserrat",
    "MT" : "Malta",
    "MU" : "Mauritius",
    "MV" : "Maldives",
    "MW" : "Malawi",
    "MX" : "Mexico",
    "MY" : "Malaysia",
    "MZ" : "Mozambique",
    "NA" : "Namibia",
    "NC" : "New Caledonia",
    "NE" : "Niger",
    "NF" : "Norfolk Island",
    "NG" : "Nigeria",
    "NI" : "Nicaragua",
    "NL" : "Netherlands",
    "NO" : "Norway",
    "NP" : "Nepal",
    "NR" : "Nauru",
    "NU" : "Niue",
    "NZ" : "New Zealand",
    "OM" : "Oman",
    "PA" : "Panama",
    "PE" : "Peru",
    "PF" : "French Polynesia",
    "PG" : "Papua New Guinea",
    "PH" : "Philippines",
    "PK" : "Pakistan",
    "PL" : "Poland",
    "PM" : "S. Pierre & Miquelon",
    "PN" : "Pitcairn Islands",
    "PR" : "Puerto Rico",
    "PS" : "Palestinian Territory",
    "PT" : "Portugal",
    "PW" : "Palau",
    "PY" : "Paraguay",
    "QA" : "Qatar",
    "RE" : "Reunion",
    "RO" : "Romania",
    "RS" : "Serbia",
    "RU" : "Russian Federation",
    "RW" : "Rwanda",
    "SA" : "Saudi Arabia",
    "SB" : "Solomon Islands",
    "SC" : "Seychelles",
    "SD" : "Sudan",
    "SE" : "Sweden",
    "SG" : "Singapore",
    "SH" : "Saint Helena",
    "SI" : "Slovenia",
    "SJ" : "Svalbard and Jan Mayen",
    "SK" : "Slovakia",
    "SL" : "Sierra Leone",
    "SM" : "San Marino",
    "SN" : "Senegal",
    "SO" : "Somalia",
    "SR" : "Suriname",
    "ST" : "Sao Tome & Principe",
    "SV" : "El Salvador",
    "SY" : "Syrian Arab Republic",
    "SZ" : "Swaziland",
    "TC" : "Turks & Caicos Islands",
    "TD" : "Chad",
    "TF" : "French S. Territories",
    "TG" : "Togo",
    "TH" : "Thailand",
    "TJ" : "Tajikistan",
    "TK" : "Tokelau",
    "TL" : "Timor-Leste",
    "TM" : "Turkmenistan",
    "TN" : "Tunisia",
    "TO" : "Tonga",
    "TR" : "Turkey",
    "TT" : "Trinidad and Tobago",
    "TV" : "Tuvalu",
    "TW" : "Taiwan",
    "TZ" : "Tanzania, Republic of",
    "UA" : "Ukraine",
    "UG" : "Uganda",
    "UM" : "US Outlying Islands",
    "US" : "United States",
    "UY" : "Uruguay",
    "UZ" : "Uzbekistan",
    "VA" : "Vatican City State",
    "VC" : "S. Vincent & Grenadines",
    "VE" : "Venezuela",
    "VG" : "Virgin Islands, British",
    "VI" : "Virgin Islands, U.S.",
    "VN" : "Vietnam",
    "VU" : "Vanuatu",
    "WF" : "Wallis and Futuna",
    "WS" : "Samoa",
    "YE" : "Yemen",
    "YT" : "Mayotte",
    "ZA" : "South Africa",
    "ZM" : "Zambia",
    "ZW" : "Zimbabwe"
}

EXTRA_OPTIONS = [
    ("EU", "Europe", "europeanunion.png"),
    ("AP", "Asia/Pacific Region"),
    ("AQ", "Antarctica"),
    ("BI", "Burundi"),
    ("BL", "Saint Barthelemy"),
    ("FX", "France, Metropolitan"),
    ("GG", "Guernsey"),
    ("IM", "Isle of Man"),
    ("JE", "Jersey"),
    ("MF", "Saint Martin"),
    ("A1", "Anonymous Proxy"),
    ("A2", "Satellite Provider"),
    ("O1", "Other")
]

CSS = "background-image: url(/static/images/flags/%s); background-repeat: no-repeat; background-position:bottom right;"


class ComboFlags (CTK.ComboCfg):
    def __init__ (self, key, _props={}):
        # Properties
        props = _props.copy()

        # Contries
        codes = ISO_3166.keys()
        codes.sort()

        countries = []
        for k in codes:
            countries.append ((k, ISO_3166[k], {'style': CSS%('%s.png'%(k))}))

        # Extra entries
        extras = []
        for k in EXTRA_OPTIONS:
            if len(k) == 2:
                extras.append ((k[0], k[1]))
            else:
                extras.append ((k[0], k[1], {'style': CSS%(k[2])}))


        options = [(_('Countries'), countries), (_('Extras'), extras)]

        CTK.ComboCfg.__init__ (self, key, options, props)


class CheckListFlags (CTK.Box):
    def __init__ (self, key, _props={}):
        CTK.Box.__init__ (self, {'class': 'check-list-flags'})

        # Flags
        props = _props.copy()
        codes = ISO_3166.keys()
        codes.sort()
        props['class'] = 'flag_checkbox'

        # Initial values
        selected = filter (lambda x: x, [x.strip() for x in CTK.cfg.get_val (key,'').split(',')])

        self += CTK.RawHTML ('<strong>%s</strong>' %(_("Countries")))
        for k in codes:
            box = CTK.Box({'class': 'check-list-flags-entry'})
            box += CTK.CheckCfg ('%s!%s' %(key, k), k in selected, props)
            box += CTK.Image ({'src': "/static/images/flags/%s.png"%(k.lower())})
            box += CTK.RawHTML (ISO_3166[k])
            self += box

        self += CTK.RawHTML ('<strong>%s</strong>' %(_("Extras")))
        for k in EXTRA_OPTIONS:
            box = CTK.Box({'class': 'check-list-flags-entry'})
            box += CTK.CheckCfg ('%s!%s' %(key, k[0]), k[0] in selected, props)
            if len(k) == 3:
                box += CTK.Image ({'src': "/static/images/flags/%s"%(k[2])})
            box += CTK.RawHTML (k[1])
            self += box
